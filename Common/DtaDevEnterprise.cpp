/* C:B**************************************************************************
   This software is Copyright (c) 2014-2024 Bright Plaza Inc. <drivetrust@drivetrust.com>
   This software is Copyright 2017 Spectra Logic Corporation

   This file is part of sedutil.

   sedutil is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   sedutil is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with sedutil.  If not, see <http://www.gnu.org/licenses/>.

   * C:E********************************************************************** */

#include <iostream>
#include "DtaDevEnterprise.h"
#include "DtaHashPwd.h"
#include "DtaEndianFixup.h"
#include "DtaCommand.h"
#include "DtaSession.h"
#include "DtaHexDump.h"
#include "DtaAnnotatedDump.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif
using namespace std;


////////////////////////////////////////////////////////////////////////////////
static const bool is_NULL_UID(std::vector<uint8_t> & v)
////////////////////////////////////////////////////////////////////////////////
{
  return
    v.size() == 9
    && v[0] == 0xa8
    && v[1] == 0x00
    && v[2] == 0x00
    && v[3] == 0x00
    && v[4] == 0x00
    && v[5] == 0x00
    && v[6] == 0x00
    && v[7] == 0x00
    && v[8] == 0x00
    ;
}

////////////////////////////////////////////////////////////////////////////////
static void setband(vector<uint8_t> & v, uint16_t i)
////////////////////////////////////////////////////////////////////////////////
{
  const uint16_t j = i+1;
  v[1+6] = (uint8_t(j >> 8) | v[1+6]) & 0xF0;
  v[1+7] = uint8_t(j);
}

////////////////////////////////////////////////////////////////////////////////
static void user2cpin(vector<uint8_t> & dst, const vector<uint8_t> & src)
////////////////////////////////////////////////////////////////////////////////
{
  // this works for BandMasterN and EraseMaster
  // Table 29 Locking C_PIN table, p. 72 of Enterprise SSC rev 3.00
  dst = src;
  dst[1+3] = 0x0B;
}

////////////////////////////////////////////////////////////////////////////////
uint8_t DtaDevEnterprise::getMaxRanges(uint16_t *maxRanges)
////////////////////////////////////////////////////////////////////////////////
{
  uint8_t lastRC;

  // 5.7.2.1.5 MaxRanges
  // This value defines the maximum number of supportable LBA ranges in addition
  // to the Global Range.  If this value is 0, then the only range available is
  // the entire Global Range of the Storage Device.
  //
  // Therefore: 0 <= supported range <= MaxRanges

  // create session
  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID)) != 0) {
    delete session;
    return lastRC;
  }

  //** Table 36 "LockingInfo table", p. 72 of Enterprise SSC rev 3.00
  vector<uint8_t> table;
  set8(table, OPALUID[ENTERPRISE_LOCKING_INFO_TABLE]);

  // query row 1 of LockingInfo table
  if (getTable(table, "MaxRanges", "MaxRanges") != 0) {
    delete session;
    // Unable to get values from Enterprise LockingInfo table -- bail out to Opal
    return getMaxRangesOpal(maxRanges);
  }
  delete session;

  // "MaxRanges" is token 5 of response
  *maxRanges = response.getUint16(5);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
uint8_t DtaDevEnterprise::getMaxRangesOpal(uint16_t *maxRanges)
////////////////////////////////////////////////////////////////////////////////
{
  uint8_t lastRC;

  // create session
  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID)) != 0) {
    delete session;
    return lastRC;
  }

  //** Table 36 "LockingInfo table", p. 72 of Enterprise SSC rev 3.00
  vector<uint8_t> table;
  set8(table, OPALUID[OPAL_LOCKING_INFO_TABLE]);

  // query row 1 of LockingInfo table
  if ((lastRC = getTable(table, "MaxRanges", "MaxRanges")) != 0) {
    LOG(E) << "Unable to get MaxRanges from LockingInfo table";
    delete session;
    return lastRC;
  }
  delete session;

  // "MaxRanges" is token 5 of response
  *maxRanges = response.getUint16(5);
  return 0;
}


uint8_t DtaDevEnterprise::initialSetup(char * password)
{
  LOG(D1) << "Entering initialSetup()";
  uint8_t lastRC;

  if ((lastRC = takeOwnership(password)) != 0) {
    LOG(E) << "Initial setup failed - unable to take ownership";
    return lastRC;
  }
  if ((lastRC = setLockingRange(0,
                                OPAL_LOCKINGSTATE::READWRITE, password)) != 0) {
    LOG(E) << "Initial setup failed - unable to unlock for read/write";
    return lastRC;
  }

  if ((lastRC = configureLockingRange(0,
                                      (DTA_READLOCKINGENABLED | DTA_WRITELOCKINGENABLED), password)) != 0) {
    LOG(E) << "Initial setup failed - unable to enable read/write locking";
    return lastRC;
  }

  LOG(D) << "Initial setup of TPer complete on " << dev;
  LOG(D1) << "Exiting initialSetup()";
  return 0;
}


uint8_t DtaDevEnterprise::initialSetup(vector<uint8_t> HostChallenge)
{
  LOG(D1) << "Entering initialSetup()";
  uint8_t lastRC;

  if ((lastRC = takeOwnership(HostChallenge)) != 0) {
    LOG(E) << "Initial setup failed - unable to take ownership";
    return lastRC;
  }
  if ((lastRC = setLockingRange(0,
                                OPAL_LOCKINGSTATE::READWRITE, HostChallenge)) != 0) {
    LOG(E) << "Initial setup failed - unable to unlock for read/write";
    return lastRC;
  }

  if ((lastRC = configureLockingRange(0,
                                      (DTA_READLOCKINGENABLED | DTA_WRITELOCKINGENABLED), HostChallenge)) != 0) {
    LOG(E) << "Initial setup failed - unable to enable read/write locking";
    return lastRC;
  }

  LOG(D) << "Initial setup of TPer complete on " << dev;
  LOG(D1) << "Exiting initialSetup()";
  return 0;
}


uint8_t DtaDevEnterprise::setup_SUM(uint8_t lockingrange, uint64_t start, uint64_t length, char *Admin1Password, char * password)
{
  LOG(D1) << "Entering DtaDevEnterprise::setup_SUM";
  LOG(D) << "setup_SUM not supported on DtaDevEnterprise";
  return 1;
}


uint8_t DtaDevEnterprise::configureLockingRange(uint8_t lockingrange, uint8_t enabled, char * password, uint8_t)
{
  uint8_t lastRC;
  LOG(D1) << "Entering DtaDevEnterprise::configureLockingRange()";

  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  std::vector<uint8_t> user;
  set8(user, OPALUID[OPAL_UID::ENTERPRISE_BANDMASTER0_UID]);
  setband(user, lockingrange);

  //** Global_Range UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
  vector<uint8_t> object;
  set8(object, OPALUID[OPAL_UID::OPAL_LOCKINGRANGE_GLOBAL]);
  setband(object, lockingrange);

  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, password, user)) != 0) {
    delete session;
    return lastRC;
  }

  /* can't use settable because the segate drives require that both the
   * read & write lockenabled be changed at the same time.  I can find no
   * written doc on such a restriction but .....
   */
  vector<uint8_t> method;
  set8(method, OPALMETHOD[OPAL_METHOD::ESET]);

  DtaCommand *set = new DtaCommand();
  if (set == NULL) {
    LOG(E) << "Unable to create command object ";
    delete session;
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  set->reset(object, method);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("ReadLockEnabled");
  set->addToken((enabled & DTA_READLOCKINGENABLED) ? OPAL_TRUE : OPAL_FALSE);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("WriteLockEnabled");
  set->addToken((enabled & DTA_WRITELOCKINGENABLED) ? OPAL_TRUE : OPAL_FALSE);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("LockOnReset");
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(UINT_00);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->complete();

  if ((lastRC = session->sendCommand(set, response)) != 0) {
    LOG(E) << "Set Failed ";
    delete session;
    delete set;
    return lastRC;
  }
  delete set;
  delete session;
  LOG(D) << "Locking range configured " << (uint16_t) enabled;
  LOG(D1) << "Exiting DtaDevEnterprise::configureLockingRange()";
  return 0;
}



uint8_t DtaDevEnterprise::configureLockingRange(uint8_t lockingrange, uint8_t enabled, vector<uint8_t> HostChallenge, uint8_t)
{
  LOG(D1) << "Entering DtaDevEnterprise::configureLockingRange()";

  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> user=vUID(OPAL_UID::ENTERPRISE_BANDMASTER0_UID);
  setband(user, lockingrange);

  //** Global_Range UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
  vector<uint8_t> object=vUID(OPAL_UID::OPAL_LOCKINGRANGE_GLOBAL);
  setband(object, lockingrange);

  uint8_t lastRC= WithSimpleSessionCommand(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, HostChallenge, user,
                                           [object, enabled](DtaCommand * set){
    vector<uint8_t> method;
    set8(method, OPALMETHOD[OPAL_METHOD::ESET]);
    set->reset(object, method);

    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::STARTNAME);
    set->addToken("ReadLockEnabled");
    set->addToken((enabled & DTA_READLOCKINGENABLED) ? OPAL_TRUE : OPAL_FALSE);
    set->addToken(OPAL_TOKEN::ENDNAME);
    set->addToken(OPAL_TOKEN::STARTNAME);
    set->addToken("WriteLockEnabled");
    set->addToken((enabled & DTA_WRITELOCKINGENABLED) ? OPAL_TRUE : OPAL_FALSE);
    set->addToken(OPAL_TOKEN::ENDNAME);
    set->addToken(OPAL_TOKEN::STARTNAME);
    set->addToken("LockOnReset");
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(UINT_00);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->addToken(OPAL_TOKEN::ENDNAME);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->addToken(OPAL_TOKEN::ENDLIST);

    set->complete();
                                           });

  if (lastRC  != 0) {
    LOG(E) << "Set Failed ";
  } else {
    LOG(D) << "Locking range configured " << (uint16_t) enabled;
    LOG(D1) << "Exiting DtaDevEnterprise::configureLockingRange()";
  }
  return lastRC;
}


uint8_t DtaDevEnterprise::rekeyLockingRange(uint8_t lockingrange, char * password)
{
  LOG(D1) << "Entering DtaDevEnterprise::rekeyLockingRange()";
  uint8_t lastRC;

  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> user;
  set8(user, OPALUID[ENTERPRISE_BANDMASTER0_UID]);
  setband(user, lockingrange);

  //** Global_Range UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
  vector<uint8_t> table;
  set8(table, OPALUID[OPAL_UID::OPAL_LOCKINGRANGE_GLOBAL]);
  setband(table, lockingrange);

  session = new DtaSession(this);
  if (NULL == session) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, password, user)) != 0) {
    delete session;
    return lastRC;
  }
  if ((lastRC = getTable(table, "ActiveKey", "ActiveKey")) != 0) {
    delete session;
    return lastRC;
  }
  std::vector<uint8_t> ActiveKey = response.getRawToken(5);
  if (is_NULL_UID(ActiveKey))
    {
      LOG(D) << "LockingRange" << (uint16_t)lockingrange << " remains in plaintext ";
      delete session;
      return 0;
    }
  DtaCommand *rekey = new DtaCommand();
  if (NULL == rekey) {
    LOG(E) << "Unable to create command object ";
    delete session;
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  rekey->reset(OPAL_UID::OPAL_AUTHORITY_TABLE, OPAL_METHOD::GENKEY);
  rekey->changeInvokingUid(ActiveKey);
  rekey->addToken(OPAL_TOKEN::STARTLIST);
  rekey->addToken(OPAL_TOKEN::ENDLIST);
  rekey->complete();
  if ((lastRC = session->sendCommand(rekey, response)) != 0) {
    LOG(E) << "rekeyLockingRange Failed ";
    delete rekey;
    delete session;
    return lastRC;
  }
  delete rekey;
  delete session;
  LOG(D) << "LockingRange" << (uint16_t)lockingrange << " reKeyed ";
  LOG(D1) << "Exiting DtaDevEnterprise::rekeyLockingRange()";
  return 0;
}
uint8_t DtaDevEnterprise::revertLockingSP(char * password, uint8_t keep)
{
  LOG(D1) << "Entering DtaDevEnterprise::revertLockingSP()";
  if(password == NULL) { LOG(D4) << "Referencing formal parameters " << keep; }
  uint8_t lastRC;
  DtaCommand *cmd = new DtaCommand();
  if (NULL == cmd) {
    LOG(E) << "Unable to create command object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  session = new DtaSession(this);
  if (NULL == session) {
    LOG(E) << "Unable to create session object ";
    delete cmd;
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  OPAL_UID uid = OPAL_UID::OPAL_SID_UID;
  if ((lastRC = session->start(OPAL_UID::OPAL_ADMINSP_UID, password, uid)) != 0) {
    delete cmd;
    delete session;
    return lastRC;
  }
  cmd->reset(OPAL_UID::OPAL_THISSP_UID, OPAL_METHOD::REVERTSP);
  cmd->addToken(OPAL_TOKEN::STARTLIST);
  cmd->addToken(OPAL_TOKEN::STARTNAME);
  cmd->addToken("KeepGlobalRangeKey");
  cmd->addToken(keep);
  cmd->addToken(OPAL_TOKEN::ENDNAME);
  cmd->addToken(OPAL_TOKEN::ENDLIST);
  cmd->complete();
  session->expectAbort();
  if ((lastRC = session->sendCommand(cmd, response)) != 0) {
    delete cmd;
    delete session;
    return lastRC;
  }
  LOG(D) << "revertLockingSP completed successfully";
  delete cmd;
  delete session;
  LOG(D1) << "Exiting DtaDevEnterprise::revertLockingSP()";
  return 0;
}

uint8_t DtaDevEnterprise::revertLockingSP(vector<uint8_t> HostChallenge, uint8_t keep)
{
  LOG(D1) << "Entering DtaDevEnterprise::revertLockingSP()";
  if(HostChallenge.size() == 0) { LOG(D4) << "Referencing formal parameters " << keep; }
  uint8_t lastRC = WithSimpleSessionCommand(OPAL_UID::OPAL_ADMINSP_UID, HostChallenge, OPAL_UID::OPAL_SID_UID,
                                            [this, keep](DtaCommand *cmd){
     cmd->reset(OPAL_UID::OPAL_THISSP_UID, OPAL_METHOD::REVERTSP);
     cmd->addToken(OPAL_TOKEN::STARTLIST);
     cmd->addToken(OPAL_TOKEN::STARTNAME);
     cmd->addToken("KeepGlobalRangeKey");
     cmd->addToken(keep);
     cmd->addToken(OPAL_TOKEN::ENDNAME);
     cmd->addToken(OPAL_TOKEN::ENDLIST);
     cmd->complete();
     session->expectAbort();
                                            });
  if (lastRC == 0) {
    LOG(D) << "revertLockingSP completed successfully";
  }
  LOG(D1) << "Exiting DtaDevEnterprise::revertLockingSP()";
  return 0;
}

uint8_t DtaDevEnterprise::setPassword(char * password, char * userid, char * newpassword, uint8_t)
{
  LOG(D1) << "Entering DtaDevEnterprise::setPassword" ;
  uint8_t lastRC;
  char *pwd = password, *newpwd = newpassword;

  if (11 > strnlen(userid, 15)) {
    LOG(E) << "Invalid Userid " << userid;
    return DTAERROR_INVALID_PARAMETER;
  }

  std::vector<uint8_t> user;
  if (!memcmp("BandMaster", userid, 10)) {
    uint16_t band = (uint16_t)atoi(&userid[10]);
    if (1023 < band) {
      LOG(E) << "Invalid Userid " << userid;
      return DTAERROR_INVALID_PARAMETER;
    }
    set8(user, OPALUID[OPAL_UID::ENTERPRISE_BANDMASTER0_UID]);
    setband(user, band);
  }
  else if (!memcmp("EraseMaster", userid, 11)) {
    set8(user, OPALUID[OPAL_UID::ENTERPRISE_ERASEMASTER_UID]);
  }
  else {
    LOG(E) << "Invalid Userid " << userid;
    return DTAERROR_INVALID_PARAMETER;
  }

  string defaultPassword;
  if ((password == NULL) || (*password == '\0') || (newpassword == NULL) ||
      (*newpassword == '\0')) {
    if ((lastRC = getMSID(defaultPassword)) != 0) {
      LOG(E) << "setPassword failed to retrieve MSID";
      return lastRC;
    }
    if ((password == NULL) || (*password == '\0'))
      pwd = (char *)defaultPassword.c_str();

    if ((newpassword == NULL) || (*newpassword == '\0'))
      newpwd = (char *)defaultPassword.c_str();
  }

  std::vector<uint8_t> usercpin;
  user2cpin(usercpin, user);
  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((password == NULL) || (*password == '\0'))
    session->dontHashPwd();
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, pwd, user)) != 0) {
    delete session;
    return lastRC;
  }

  if ((newpassword == NULL) || (*newpassword == '\0')) {
    std::vector<uint8_t> tmppwd;

    tmppwd.push_back(0xd0);
    tmppwd.push_back((uint8_t)strnlen(newpwd, 255));
    for (unsigned int i = 0; i < strnlen(newpwd, 255); i++) {
      tmppwd.push_back(newpwd[i]);
    }

    if ((lastRC = setTable(usercpin, "PIN", tmppwd)) != 0) {
      LOG(E) << "Unable to set user " << userid << " new password ";
      delete session;
      return lastRC;
    }
  } else {
    std::vector<uint8_t> hash;
    DtaHashPwd(hash, newpwd, this);
    if ((lastRC = setTable(usercpin, "PIN", hash)) != 0) {
      LOG(E) << "Unable to set user " << userid << " new password ";
      delete session;
      return lastRC;
    }
  }

  LOG(I) << userid << " password changed";
  delete session;
  LOG(D1) << "Exiting DtaDevEnterprise::setPassword()";
  return 0;
}



uint8_t DtaDevEnterprise::setHostChallenge(vector<uint8_t> currentHostChallenge, char * userid, vector<uint8_t> newHostChallenge, uint8_t)
{
  LOG(D1) << "Entering DtaDevEnterprise::setPassword" ;

  if (11 > strnlen(userid, 15)) {
    LOG(E) << "Invalid Userid " << userid;
    return DTAERROR_INVALID_PARAMETER;
  }

  std::vector<uint8_t> user;
  if (!memcmp("BandMaster", userid, 10)) {
    uint16_t band = (uint16_t)atoi(&userid[10]);
    if (1023 < band) {
      LOG(E) << "Invalid Userid " << userid;
      return DTAERROR_INVALID_PARAMETER;
    }
    set8(user, OPALUID[OPAL_UID::ENTERPRISE_BANDMASTER0_UID]);
    setband(user, band);
  }
  else if (!memcmp("EraseMaster", userid, 11)) {
    set8(user, OPALUID[OPAL_UID::ENTERPRISE_ERASEMASTER_UID]);
  }
  else {
    LOG(E) << "Invalid Userid " << userid;
    return DTAERROR_INVALID_PARAMETER;
  }

  uint8_t lastRC;

  if (currentHostChallenge.size() == 0 || newHostChallenge.size() == 0) {
    string defaultPassword;
    if ((lastRC = getMSID(defaultPassword)) != 0) {
      LOG(E) << "setPassword failed to retrieve MSID";
      return lastRC;
    }
    const char * dps = defaultPassword.c_str();
    uint8_t dpslen = (uint8_t)strlen(dps);
    vector<uint8_t> defaultHostChallenge(dps, dps+dpslen);
    if (currentHostChallenge.size() == 0)
      currentHostChallenge=defaultHostChallenge;
    if (newHostChallenge.size() == 0)
      newHostChallenge=defaultHostChallenge;
  }

  std::vector<uint8_t> usercpin;
  user2cpin(usercpin, user);

  lastRC = WithSession([this, currentHostChallenge, user](){
    return session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, currentHostChallenge, user);
  },
    [this, newHostChallenge, usercpin](){
      vector<uint8_t>token=newHostChallenge;
      token.insert(token.begin(), (uint8_t)newHostChallenge.size());
      token.insert(token.begin(), 0xd0);
      return setTable(usercpin, "PIN", token);
    });

  if (lastRC == 0) {
    LOG(I) << userid << " password changed";
  }
  LOG(D1) << "Exiting DtaDevEnterprise::setPassword()";
  return lastRC;
}

uint8_t DtaDevEnterprise::setNewPassword_SUM(char * password, char * userid, char * newpassword)
{
  LOG(D1) << "Entering DtaDevEnterprise::setNewPassword_SUM()";
  LOG(D) << "setNewPassword_SUM is not in the Enterprise SSC and not supported";
  LOG(D1) << "Exiting DtaDevEnterprise::setNewPassword_SUM()";
  return 0;
}

uint8_t DtaDevEnterprise::setMBREnable(uint8_t mbrstate,    char * Admin1Password)
{
  LOG(D1) << "Entering DtaDevEnterprise::setMBREnable";
  if (NULL == Admin1Password) { LOG(E) << "This shouldn't happen " << mbrstate; }
  LOG(D) << "MBR shadowing is optional in the Enterprise SSC and not supported";
  LOG(D1) << "Exiting DtaDevEnterprise::setMBREnable";
  return 0;
}
uint8_t DtaDevEnterprise::setMBREnable(uint8_t state, vector<uint8_t> Admin1HostChallenge)
{
  LOG(D1) << "Entering DtaDevEnterprise::setMBREnable";
  if (0 == Admin1HostChallenge.size()) { LOG(E) << "This shouldn't happen " << state; }
  LOG(D) << "MBR shadowing is optional in the Enterprise SSC and not supported";
  LOG(D1) << "Exiting DtaDevEnterprise::setMBREnable";
  return 0;
}

uint8_t DtaDevEnterprise::setMBRDone(uint8_t mbrstate, char * Admin1Password)
{
  LOG(D1) << "Entering DtaDevEnterprise::setMBRDone";
  if (NULL == Admin1Password) { LOG(E) << "This shouldn't happen " << mbrstate; }
  LOG(D) << "MBR shadowing is optional in the Enterprise SSC and not supported";
  LOG(D1) << "Exiting DtaDevEnterprise::setMBRDone";
  return 0;
}
uint8_t DtaDevEnterprise::setMBRDone(uint8_t state, vector<uint8_t> Admin1HostChallenge)
{
  LOG(D1) << "Entering DtaDevEnterprise::setMBRDone";
  if (0 == Admin1HostChallenge.size()) { LOG(E) << "This shouldn't happen " << state; }
  LOG(D) << "MBR shadowing is optional in the Enterprise SSC and not supported";
  LOG(D1) << "Exiting DtaDevEnterprise::setMBRDone";
  return 0;
}

uint8_t DtaDevEnterprise::TCGreset(uint8_t mbrstate)
{
  LOG(D1) << "Entering DtaDevEnterprise::TCGreset";

  LOG(D1) << "Exiting DtaDevEnterprise::TCGreset";
  return 0;
}

uint8_t DtaDevEnterprise::setupLockingRange(uint8_t lockingrange, uint64_t start, uint64_t length, char * password)
{
  uint8_t lastRC;
  LOG(D1) << "Entering DtaDevEnterprise::setupLockingRange";
  // look up MaxRanges
  uint16_t MaxRanges;

  if ((lastRC = getMaxRanges(&MaxRanges)) != 0) {
    return (lastRC);
  }
  if (MaxRanges == 0 || MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  // make sure lockingrange is in bounds
  if (lockingrange > MaxRanges)
    {
      LOG(E) << "Requested locking range " << lockingrange << " greater than MaxRanges " << MaxRanges;
      return DTAERROR_UNSUPORTED_LOCKING_RANGE;
    }

  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> user;
  set8(user, OPALUID[OPAL_UID::ENTERPRISE_BANDMASTER0_UID]);
  setband(user, lockingrange);

  //** Global_Range UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
  vector<uint8_t> object;
  set8(object, OPALUID[OPAL_UID::OPAL_LOCKINGRANGE_GLOBAL]);
  setband(object, lockingrange);

  vector<uint8_t> method;
  set8(method, OPALMETHOD[OPAL_METHOD::ESET]);

  DtaCommand *set = new DtaCommand();
  set->reset(object, method);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("RangeStart");
  set->addToken(start);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("RangeLength");
  set->addToken(length);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("ReadLockEnabled");
  set->addToken(OPAL_TOKEN::OPAL_FALSE);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("WriteLockEnabled");
  set->addToken(OPAL_TOKEN::OPAL_FALSE);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("ReadLocked");
  set->addToken(OPAL_TOKEN::OPAL_FALSE);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("WriteLocked");
  set->addToken(OPAL_TOKEN::OPAL_FALSE);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("LockOnReset");
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->complete();

  session = new DtaSession(this);
  if (NULL == session) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, password, user)) != 0) {
    delete session;
    return lastRC;
  }
  if ((lastRC = session->sendCommand(set, response)) != 0) {
    LOG(E) << "setupLockingRange Failed ";
    delete set;
    delete session;
    return lastRC;
  }
  delete set;
  delete session;
  LOG(D) << "LockingRange" << (uint16_t)lockingrange << " starting block " << start <<
    " for " << length << " blocks configured as unlocked range";
  LOG(D1) << "Exiting DtaDevEnterprise::setupLockingRange";
  return 0;
}
uint8_t DtaDevEnterprise::setupLockingRange_SUM(uint8_t lockingrange, uint64_t start,
                                                uint64_t length, char * password) {
  LOG(D1) << "Entering DtaDevEnterprise::setupLockingRange_SUM";
  if (0 == lockingrange) { LOG(E) << start << length << password; }
  LOG(D1) << "Exiting DtaDevEnterprise::setupLockingRange_SUM";
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
uint8_t DtaDevEnterprise::listLockingRanges(char * password, int16_t rangeid, uint8_t)
////////////////////////////////////////////////////////////////////////////////
{
  LOG(D1) << "Entering DtaDevEnterprise::listLockingRanges";
  uint8_t lastRC = 0, failRC = 0;
  int one_succeeded = 0;
  char *pwd = NULL;

  // if (NULL == password) { LOG(E) << "password NULL"; }
  string defaultPassword;
  if ((password == NULL) || (*password == '\0')) {

    if ((lastRC = getMSID(defaultPassword)) != 0) {
      LOG(E) << __func__ << ": unable to retrieve MSID";
      return lastRC;
    }
    pwd = (char *)defaultPassword.c_str();
  } else {
    pwd = password;
  }

  // look up MaxRanges
  uint16_t MaxRanges;

  if (rangeid == -1) {
    lastRC = getMaxRanges(&MaxRanges);
    if (lastRC != 0)
      return lastRC;
  } else
    MaxRanges = rangeid;

  if (MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  if(rangeid == -1) {
    LOG(I) << "Maximum ranges supported: " << MaxRanges;
  }

  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> user;
  user.clear();
  set8(user, OPALUID[ENTERPRISE_BANDMASTER0_UID]);

  //** Global_Range UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
  vector<uint8_t> table;
  table.clear();
  set8(table, OPALUID[OPAL_LOCKINGRANGE_GLOBAL]);

  uint16_t start = (rangeid == -1)? 0: rangeid;
  for (uint16_t i = start; i <= MaxRanges; i++)
    {
      uint8_t curRC = 0;

      setband(user, i);
      setband(table, i);

      if (outputFormat == sedutilNormal) {
        LOG(I) << "Band[" << i << "]:";
      }

      session = new DtaSession(this);
      if (session == NULL) {
        LOG(E) << "Unable to create session object ";
        return DTAERROR_OBJECT_CREATE_FAILED;
      }
      if (!defaultPassword.empty())
        session->dontHashPwd();

      if ((curRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, pwd, user)) != 0)
        {
          if ((outputFormat == sedutilNormal) || (rangeid != -1)) {
            LOG(I) << "    could not establish session for row[" << i << "]";
          }
          failRC = curRC;
          delete session;
          continue;
        }
      if (getTable(table, "Name", "LockOnReset"))
        {
          LOG(D) << "    row[" << i << "] not found in LOCKING table";
          delete session;
          continue;
        }
      // 00	1   ( F0 ) Start_List
      // 01	1   ( F0 ) Start_List
      // 02	1   ( F0 ) Start_List
      // 03	1   ( F2 ) Start_Name
      // 04	5   ( A4 ) 4E 61 6D 65 ("Name")
      // 05*	6   ( A5 ) 42 61 6E 64 31 ("Band1")
      // 06	1   ( F3 ) End_Name
      // 07	1   ( F2 ) Start_Name
      // 08	11  ( AA ) 43 6F 6D 6D 6F 6E 4E 61 6D 65 ("CommonName")
      // 09*	8   ( A7 ) 4C 6F 63 6B 69 6E 67 ("Locking")
      // 10	1   ( F3 ) End_Name
      // 11	1   ( F2 ) Start_Name
      // 12	11  ( AA ) 52 61 6E 67 65 53 74 61 72 74 ("RangeStart")
      // 13*	1   ( 00 ) 0 (0h)
      // 14	1   ( F3 ) End_Name
      // 15	1   ( F2 ) Start_Name
      // 16	12  ( AB ) 52 61 6E 67 65 4C 65 6E 67 74 68 ("RangeLength")
      // 17*	1   ( 00 ) 0 (0h)
      // 18	1   ( F3 ) End_Name
      // 19	1   ( F2 ) Start_Name
      // 20	16  ( AF ) 52 65 61 64 4C 6F 63 6B 45 6E 61 62 6C 65 64 ("ReadLockEnabled")
      // 21*	1   ( 00 ) 0 (0h)
      // 22	1   ( F3 ) End_Name
      // 23	1   ( F2 ) Start_Name
      // 24	18  ( D0 10 ) 57 72 69 74 65 4C 6F 63 6B 45 6E 61 62 6C 65 64 ("WriteLockEnabled")
      // 25*	1   ( 00 ) 0 (0h)
      // 26	1   ( F3 ) End_Name
      // 27	1   ( F2 ) Start_Name
      // 28	11  ( AA ) 52 65 61 64 4C 6F 63 6B 65 64 ("ReadLocked")
      // 29*	1   ( 01 ) 1 (1h)
      // 30	1   ( F3 ) End_Name
      // 31	1   ( F2 ) Start_Name
      // 32	12  ( AB ) 57 72 69 74 65 4C 6F 63 6B 65 64 ("WriteLocked")
      // 33*	1   ( 01 ) 1 (1h)
      // 34	1   ( F3 ) End_Name
      // 35	1   ( F2 ) Start_Name
      // 36	12  ( AB ) 4C 6F 63 6B 4F 6E 52 65 73 65 74 ("LockOnReset")
      // 37	1   ( F0 ) Start_List
      // 38	1   ( 00 ) 0 (0h)
      // 39	1   ( F1 ) End_List
      // 40	1   ( F3 ) End_Name
      const std::string Name          = response.getString(5+4*0);
      const std::string CommonName    = response.getString(5+4*1);
      const uint64_t RangeStart       = response.getUint64(5+4*2);
      const uint64_t RangeLength      = response.getUint64(5+4*3);
      const bool ReadLockEnabled      = response.getUint8(5+4*4) != 0;
      const bool WriteLockEnabled     = response.getUint8(5+4*5) != 0;
      const bool ReadLocked           = response.getUint8(5+4*6) != 0;
      const bool WriteLocked          = response.getUint8(5+4*7) != 0;
      // LockOnReset list has at least one element
      const bool LockOnReset          = response.tokenIs(5+4*8) == STARTLIST &&
                                        response.tokenIs(5+4*8+1) == DTA_TOKENID_UINT;
      delete session;

      if (outputFormat == sedutilReadable) {
        LOG(I) << "Band[" << i << "]: ";
      }

      LOG(I) << "    Name:            " << Name;
      LOG(I) << "    CommonName:      " << CommonName;
      LOG(I) << "    RangeStart:      " << RangeStart;
      LOG(I) << "    RangeLength:     " << RangeLength;
      LOG(I) << "    ReadLockEnabled: " << ReadLockEnabled;
      LOG(I) << "    WriteLockEnabled:" << WriteLockEnabled;
      LOG(I) << "    ReadLocked:      " << ReadLocked;
      LOG(I) << "    WriteLocked:     " << WriteLocked;
      LOG(I) << "    LockOnReset:     " << LockOnReset;

      one_succeeded = 1;
    }

  // If we're getting the list of ranges and none succeed, that is an error.
  // If we're getting one range, return any failure.
  if (((rangeid == -1) && (one_succeeded == 0))
      || (rangeid != -1))
    lastRC = failRC;

  LOG(D1) << "Exiting DtaDevEnterprise::listLockingRanges";
  return lastRC;
}

uint8_t DtaDevEnterprise::setLockingRange(uint8_t lockingrange, uint8_t lockingstate,
                                          char * password, uint8_t)
{
  LOG(D1) << "Entering DtaDevEnterprise::setLockingRange";
  uint8_t lastRC;

  // convert Opal lockingstate to boolean
  OPAL_TOKEN locked;
  switch (lockingstate) {
  case OPAL_LOCKINGSTATE::READWRITE:
    locked = OPAL_TOKEN::OPAL_FALSE;
    break;
  case OPAL_LOCKINGSTATE::READONLY:
    LOG(E) << "Read Only locking state is unsupported in Enterprise SSC";
    return DTAERROR_INVALID_PARAMETER;
  case OPAL_LOCKINGSTATE::LOCKED:
    locked = OPAL_TOKEN::OPAL_TRUE;
    break;
  default:
    LOG(E) << "Invalid locking state for setLockingRange (RW=1, LOCKED=3)";
    return DTAERROR_INVALID_PARAMETER;
  }

  // look up MaxRanges
  uint16_t MaxRanges;

  if ((lastRC = getMaxRanges(&MaxRanges)) != 0) {
    return lastRC;
  }
  if (MaxRanges == 0 || MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  // make sure lockingrange is in bounds
  if (lockingrange > MaxRanges)
    {
      LOG(E) << "Requested locking range " << lockingrange << " greater than MaxRanges " << MaxRanges;
      return DTAERROR_UNSUPORTED_LOCKING_RANGE;
    }

  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> user;
  set8(user, OPALUID[OPAL_UID::ENTERPRISE_BANDMASTER0_UID]);
  setband(user, lockingrange);

  //** Band0 UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
  vector<uint8_t> object;
  set8(object, OPALUID[OPAL_UID::OPAL_LOCKINGRANGE_GLOBAL]);
  setband(object, lockingrange);

  vector<uint8_t> method;
  set8(method, OPALMETHOD[OPAL_METHOD::ESET]);

  DtaCommand *set = new DtaCommand();
  set->reset(object, method);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("WriteLocked");
  set->addToken(locked);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken("ReadLocked");
  set->addToken(locked);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->complete();
  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, password, user))
      != 0) {
    delete session;
    return lastRC;
  }
  if ((lastRC = session->sendCommand(set, response)) != 0) {
    LOG(E) << "Set Failed ";
    delete session;
    delete set;
    return lastRC;
  }
  delete set;
  delete session;
  LOG(D) << "Locking range Read/Write set " << (uint16_t)locked;
  LOG(D1) << "Exiting DtaDevEnterprise::setLockingRange";
  return 0;
}


uint8_t DtaDevEnterprise::setLockingRange(uint8_t lockingrange, uint8_t lockingstate,
                                          vector<uint8_t>HostChallenge, uint8_t){
  LOG(D1) << "Entering DtaDevEnterprise::setLockingRange";
  uint8_t lastRC;

  // convert Opal lockingstate to boolean
  OPAL_TOKEN locked;
  switch (lockingstate) {
  case OPAL_LOCKINGSTATE::READWRITE:
    locked = OPAL_TOKEN::OPAL_FALSE;
    break;
  case OPAL_LOCKINGSTATE::READONLY:
    LOG(E) << "Read Only locking state is unsupported in Enterprise SSC";
    return DTAERROR_INVALID_PARAMETER;
  case OPAL_LOCKINGSTATE::LOCKED:
    locked = OPAL_TOKEN::OPAL_TRUE;
    break;
  default:
    LOG(E) << "Invalid locking state for setLockingRange (RW=1, LOCKED=3)";
    return DTAERROR_INVALID_PARAMETER;
  }

  // look up MaxRanges
  uint16_t MaxRanges;

  if ((lastRC = getMaxRanges(&MaxRanges)) != 0) {
    return lastRC;
  }
  if (MaxRanges == 0 || MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  // make sure lockingrange is in bounds
  if (lockingrange > MaxRanges)
    {
      LOG(E) << "Requested locking range " << lockingrange << " greater than MaxRanges " << MaxRanges;
      return DTAERROR_UNSUPORTED_LOCKING_RANGE;
    }


  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> user = vUID(OPAL_UID::ENTERPRISE_BANDMASTER0_UID);
  setband(user, lockingrange);
  lastRC = WithSimpleSessionCommand(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, HostChallenge, user,
                                    [lockingrange, locked](DtaCommand * set){

    //** Band0 UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
    vector<uint8_t> object = vUID(OPAL_UID::OPAL_LOCKINGRANGE_GLOBAL);
    setband(object, lockingrange);

    vector<uint8_t> method;
    set8(method, OPALMETHOD[OPAL_METHOD::ESET]);

    set->reset(object, method);
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::STARTLIST);
    set->addToken(OPAL_TOKEN::STARTNAME);
    set->addToken("WriteLocked");
    set->addToken(locked);
    set->addToken(OPAL_TOKEN::ENDNAME);
    set->addToken(OPAL_TOKEN::STARTNAME);
    set->addToken("ReadLocked");
    set->addToken(locked);
    set->addToken(OPAL_TOKEN::ENDNAME);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->addToken(OPAL_TOKEN::ENDLIST);
    set->complete();
                                                                            });

  if (lastRC != 0) {
    LOG(E) << "DtaDevEnterprise::setLockingRange:: SET Failed ";
  } else {
    LOG(D) << "Locking range Read/Write set " << (uint16_t)locked;
    LOG(D1) << "Exiting DtaDevEnterprise::setLockingRange";
  }

  return lastRC;
}


uint8_t DtaDevEnterprise::setLockingRange_SUM(uint8_t lockingrange, uint8_t lockingstate,
                                              char * password) {
  LOG(D1) << "Entering DtaDevEnterprise::setLockingRange_SUM()";
  LOG(E) << "setLockingRange_SUM not implemented";
  LOG(D1) << "Exiting DtaDevEnterprise::setLockingRange_SUM()";
  return DTAERROR_INVALID_PARAMETER;
}

uint8_t DtaDevEnterprise::enableUser(uint8_t mbrstate, char * password, char * userid)
{
  LOG(D1) << "Entering DtaDevEnterprise::enableUser";
  LOG(E) << "enableUser not implemented";
  if (!password || !userid) { LOG(E) << "Formal Parameters"; }
  LOG(D1) << "Exiting DtaDevEnterprise::enableUser()";
  return DTAERROR_INVALID_PARAMETER;
}
uint8_t DtaDevEnterprise::enableUser(uint8_t mbrstate, vector<uint8_t> HostChallenge, char * userid)
{
  LOG(D1) << "Entering DtaDevEnterprise::enableUser";
  LOG(E) << "enableUser not implemented";
  if (0==HostChallenge.size() || !userid) { LOG(E) << "Formal Parameters"; }
  LOG(D1) << "Exiting DtaDevEnterprise::enableUser()";
  return DTAERROR_INVALID_PARAMETER;
}

//uint8_t DtaDevEnterprise::enableUser(char * password, char * userid, OPAL_TOKEN status)
//{
//	LOG(D1) << "Entering DtaDevEnterprise::enableUser";
//	LOG(E) << "enableUser not implemented";
//	if (!password || !userid) { LOG(E) << "Formal Parameters"; }
//	LOG(D1) << "Exiting DtaDevEnterprise::enableUser()";
//	return 0xff;
//}
uint8_t DtaDevEnterprise::enableUserRead(uint8_t mbrstate, char * password, char * userid)
{
  LOG(D1) << "Entering DtaDevEnterprise::enableUserRead";
  LOG(E) << "enableUserRead not implemented";
  if (!password || !userid) { LOG(E) << "Formal Parameters"; }
  LOG(D1) << "Exiting DtaDevEnterprise::enableUserRead()";
  return 0xff;
}
uint8_t DtaDevEnterprise::enableUserRead(uint8_t state, vector<uint8_t> HostChallenge, char * userid)
{
  LOG(D1) << "Entering DtaDevEnterprise::enableUserRead";
  LOG(E) << "enableUserRead not implemented";
  if (0==HostChallenge.size() || !userid) { LOG(E) << "Formal Parameters"; }
  LOG(D1) << "Exiting DtaDevEnterprise::enableUserRead()";
  return 0xff;
}


uint8_t DtaDevEnterprise::revertTPer(char * password, uint8_t PSID, uint8_t AdminSP)
{
  LOG(D1) << "Entering DtaDevEnterprise::revertTPer()";
  if (password == NULL) { LOG(D4) << "Referencing formal parameters " << PSID; }
  uint8_t lastRC;
  DtaCommand *cmd = new DtaCommand();
  if (NULL == cmd) {
    LOG(E) << "Unable to create command object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  session = new DtaSession(this);
  if (NULL == session) {
    LOG(E) << "Unable to create session object ";
    delete cmd;
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  OPAL_UID uid = OPAL_UID::OPAL_SID_UID;
  if (PSID) {
    session->dontHashPwd(); // PSID pwd should be passed as entered
    uid = OPAL_UID::OPAL_PSID_UID;
  }
  if ((lastRC = session->start(OPAL_UID::OPAL_ADMINSP_UID, password, uid)) != 0) {
    delete cmd;
    delete session;
    return lastRC;
  }
  if (AdminSP)
    cmd->reset(OPAL_UID::OPAL_ADMINSP_UID, OPAL_METHOD::REVERT);
  else
    cmd->reset(OPAL_UID::OPAL_THISSP_UID, OPAL_METHOD::REVERTSP);
  cmd->addToken(OPAL_TOKEN::STARTLIST);
  cmd->addToken(OPAL_TOKEN::ENDLIST);
  cmd->complete();
  session->expectAbort();
  if ((lastRC = session->sendCommand(cmd, response)) != 0) {
    delete cmd;
    delete session;
    return lastRC;
  }
  LOG(D) << "revertTper completed successfully";
  delete cmd;
  delete session;
  LOG(D1) << "Exiting DtaDevEnterprise::revertTPer()";
  return 0;
}

uint8_t DtaDevEnterprise::revertTPer(vector<uint8_t> HostChallenge, uint8_t PSID, uint8_t AdminSP)
{
  LOG(D1) << "Entering DtaDevEnterprise::revertTPer()";
  if (HostChallenge.size() == 0) { LOG(D4) << "Referencing formal parameters " << PSID; }
  uint8_t lastRC = WithSimpleSessionCommand(OPAL_UID::OPAL_ADMINSP_UID, HostChallenge,
                                            PSID ? OPAL_UID::OPAL_PSID_UID : OPAL_UID::OPAL_SID_UID ,
                                            [this, AdminSP](DtaCommand * cmd){
     if (AdminSP)
       cmd->reset(OPAL_UID::OPAL_ADMINSP_UID, OPAL_METHOD::REVERT);
     else
       cmd->reset(OPAL_UID::OPAL_THISSP_UID, OPAL_METHOD::REVERTSP);
     cmd->addToken(OPAL_TOKEN::STARTLIST);
     cmd->addToken(OPAL_TOKEN::ENDLIST);
     cmd->complete();
     session->expectAbort();
                                                                             });
  if (lastRC == 0) {
    LOG(D) << "revertTper completed successfully";
  }
  LOG(D1) << "Exiting DtaDevEnterprise::revertTPer()";
  return lastRC;
}

uint8_t DtaDevEnterprise::eraseLockingRange(uint8_t lockingrange, char * password)
{
  uint8_t lastRC;
  char *pwd = NULL;
  LOG(D1) << "Entering DtaDevEnterprise::eraseLockingRange";

  // look up MaxRanges
  uint16_t MaxRanges = 0;


  string defaultPassword;

  if ((password == NULL) || (*password == '\0')) {
    if ((lastRC = getMSID(defaultPassword)) != 0) {
      LOG(E) << __func__ << ": unable to retrieve MSID";
      return lastRC;
    }
    pwd = (char *)defaultPassword.c_str();
  } else {
    pwd = password;
  }

  if ((lastRC = getMaxRanges(&MaxRanges)) != 0) {
    return lastRC;
  }
  if (MaxRanges == 0 || MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  // make sure lockingrange is in bounds
  if (lockingrange > MaxRanges)
    {
      LOG(E) << "Requested locking range " << lockingrange << " greater than MaxRanges " << MaxRanges;
      return DTAERROR_UNSUPORTED_LOCKING_RANGE;
    }

  //** EraseMaster UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> user;
  set8(user, OPALUID[OPAL_UID::ENTERPRISE_ERASEMASTER_UID]);

  //** Band0 UID of Table 33 Locking SP Locking table, p. 84 of Enterprise SSC rev 3.00
  vector<uint8_t> object;
  set8(object, OPALUID[OPAL_UID::OPAL_LOCKINGRANGE_GLOBAL]);
  setband(object, lockingrange);

  vector<uint8_t> method;
  set8(method, OPALMETHOD[OPAL_METHOD::ERASE]);

  DtaCommand *erase = new DtaCommand();
  if (erase == NULL) {
    LOG(E) << "Unable to create command object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  erase->reset(object, method);
  erase->addToken(OPAL_TOKEN::STARTLIST);
  erase->addToken(OPAL_TOKEN::ENDLIST);
  erase->complete();

  session = new DtaSession(this);
  if (NULL == session) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if (!defaultPassword.empty())
    session->dontHashPwd();

  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, pwd, user)) != 0) {
    delete session;
    return lastRC;
  }
  if ((lastRC = session->sendCommand(erase, response)) != 0) {
    LOG(E) << "eraseLockingRange Failed ";
    delete erase;
    delete session;
    return lastRC;
  }
  delete erase;
  delete session;
  LOG(D) << "LockingRange" << (uint16_t)lockingrange << " erased";
  LOG(D1) << "Exiting DtaDevEnterprise::eraseLockingRange";
  return 0;
}
uint8_t DtaDevEnterprise::activate(char * password) {
  LOG(D1) << "Entering DtaDevEnterprise::activate() not implemented in Enterprize";
  return 0;
}
uint8_t DtaDevEnterprise::getmfgstate() {
  LOG(D1) << "Entering DtaDevEnterprise::getmfgstate() not implemented in Enterprize";
  return 0;
}
uint8_t DtaDevEnterprise::getMBRsize(char * password) {
  LOG(D1) << "Entering DtaDevEnterprise::getMBRsize() not implemented in Enterprize";
  return 0;
}


uint8_t DtaDevEnterprise::loadPBA(char * password, char * filename) {
  LOG(D1) << "Entering DtaDevEnterprise::loadPBAimage()" << filename << " " << dev;
  if (password == NULL) { LOG(D4) << "Referencing formal parameters " << filename; }
  LOG(D) << "loadPBA is not implemented.  It is not a mandatory part of  ";
  LOG(D) << "the enterprise SSC ";
  LOG(D1) << "Exiting DtaDevEnterprise::loadPBAimage()";
  return DTAERROR_INVALID_PARAMETER;
}
uint8_t DtaDevEnterprise::activateLockingSP(char * password)
{
  LOG(D1) << "Entering DtaDevEnterprise::activateLockingSP()";
  if (password == NULL) { LOG(D4) << "Referencing formal parameters "; }
  LOG(E) << "activate Locking SP is not a part of the Enterprise SSC ";
  LOG(D1) << "Exiting DtaDevEnterprise::activatLockingSP()";
  return DTAERROR_INVALID_PARAMETER;
}
uint8_t DtaDevEnterprise::activateLockingSP(vector<uint8_t>HostChallenge)
{
  LOG(D1) << "Entering DtaDevEnterprise::activateLockingSP()";
  if (0 == HostChallenge.size()) { LOG(D4) << "Referencing formal parameters "; }
  LOG(E) << "activate Locking SP is not a part of the Enterprise SSC ";
  LOG(D1) << "Exiting DtaDevEnterprise::activatLockingSP()";
  return DTAERROR_INVALID_PARAMETER;
}


uint8_t DtaDevEnterprise::activateLockingSP_SUM(uint8_t lockingrange, char * password)
{
  LOG(D1) << "Entering DtaDevEnterprise::activateLockingSP_SUM()";
  if (password == NULL) { LOG(D4) << "Referencing formal parameters "; }
  LOG(E) << "activate Locking SP SUM is not a part of the Enterprise SSC ";
  LOG(D1) << "Exiting DtaDevEnterprise::activateLockingSP_SUM()";
  return DTAERROR_INVALID_PARAMETER;
}
uint8_t DtaDevEnterprise::eraseLockingRange_SUM(uint8_t lockingrange, char * password)
{
  LOG(D1) << "Entering DtaDevEnterprise::eraseLockingRange_SUM()";
  if (password == NULL) { LOG(D4) << "Referencing formal parameters "; }
  LOG(E) << "Erase Locking Range SUM is not a part of the Enterprise SSC ";
  LOG(D1) << "Exiting DtaDevEnterprise::eraseLockingRange_SUM()";
  return DTAERROR_INVALID_PARAMETER;
}
uint8_t DtaDevEnterprise::takeOwnership(char * newpassword)
{
  uint8_t lastRC;

  LOG(D1) << "Entering DtaDevEnterprise::takeOwnership()";
  string defaultPassword;
  if ((lastRC = getMSID(defaultPassword)) != 0) {
    LOG(E) << "takeOwnership failed unable to retrieve MSID";
    return lastRC;
  }
  if ((lastRC = setSIDPassword((char *)defaultPassword.c_str(), newpassword, 0)) != 0) {
    LOG(E) << "takeOwnership failed unable to set new SID password";
    return lastRC;
  }
  if ((lastRC = initLSPUsers((char *)defaultPassword.c_str(), newpassword)) != 0) {
    LOG(E) << "takeOwnership failed unable to set Locking SP user passwords";
    return lastRC;
  }
  LOG(D) << "takeOwnership complete";
  LOG(D1) << "Exiting takeOwnership()";
  return 0;
}

uint8_t DtaDevEnterprise::takeOwnership(vector<uint8_t> HostChallenge)
{
  uint8_t lastRC;

  LOG(D1) << "Entering DtaDevEnterprise::takeOwnership()";
  string defaultPassword;
  if ((lastRC = getMSID(defaultPassword)) != 0) {
    LOG(E) << "takeOwnership failed unable to retrieve MSID";
    return lastRC;
  }
  const char * dps = defaultPassword.c_str();
  vector<uint8_t> defaultHostChallenge(dps, dps+strlen(dps));
  defaultHostChallenge.resize(8, (uint8_t)0);
  if ((lastRC = setSIDHostChallenge(defaultHostChallenge, HostChallenge)) != 0) {
    LOG(E) << "takeOwnership failed unable to set new SID password";
    return lastRC;
  }
  if ((lastRC = initLSPUsers(const_cast<char *>(dps), HostChallenge)) != 0) {
    LOG(E) << "takeOwnership failed unable to set Locking SP user passwords";
    return lastRC;
  }
  LOG(D) << "takeOwnership complete";
  LOG(D1) << "Exiting takeOwnership()";
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
uint8_t DtaDevEnterprise::setBandsEnabled(int16_t lockingrange, char * password)
////////////////////////////////////////////////////////////////////////////////
{
  uint8_t lastRC = 0;
  LOG(D1) << "Entering DtaDevEnterprise::eraseLockingRange";

  // look up MaxRanges
  uint16_t MaxRanges = 0;

  if ((lastRC = getMaxRanges(&MaxRanges)) != 0) {
    return lastRC;
  }
  if (MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  // calculate starting and ending bands
  int lo, hi;
  if (lockingrange < 0)
    {
      lo = 0;
      hi = MaxRanges;
    }
  else if (lockingrange > MaxRanges)
    {
      LOG(E) << "Requested locking range " << lockingrange << " greater than MaxRanges " << MaxRanges;
      return DTAERROR_UNSUPORTED_LOCKING_RANGE;
    }
  else
    {
      lo = lockingrange;
      hi = lockingrange;
    }

  // get password (usually MSID)
  string pwd;
  const bool useMSID = password == NULL || *password == '\0';
  if (!useMSID) {
    pwd = password;
  } else if ((lastRC = getMSID(pwd)) != 0) {
    return lastRC;
  }

  vector<uint8_t> erasemaster;
  set8(erasemaster, OPALUID[OPAL_UID::ENTERPRISE_ERASEMASTER_UID]);

  //** BandMaster0 UID of Table 28 Locking SP Authority table, p. 70 of Enterprise SSC rev 3.00
  vector<uint8_t> object;
  set8(object, OPALUID[OPAL_UID::ENTERPRISE_BANDMASTER0_UID]);

  // method is enterprise set
  vector<uint8_t> method;
  set8(method, OPALMETHOD[OPAL_METHOD::ESET]);

  // set Enabled=TRUE in BandMaster[n] row of (Table 28) Locking SP Authority
  for(int n=lo; n<=hi && lastRC==0; n++)
    {
      // BandMaster[n] row
      setband(object, (uint16_t) n);

      // command to set Enabled column
      DtaCommand *cmd = new DtaCommand();
      if (cmd == NULL) {
        LOG(E) << "Unable to create command object ";
        return DTAERROR_OBJECT_CREATE_FAILED;
      }
      cmd->reset(object, method);
      cmd->addToken(OPAL_TOKEN::STARTLIST);
      cmd->addToken(OPAL_TOKEN::STARTLIST);
      cmd->addToken(OPAL_TOKEN::ENDLIST);
      cmd->addToken(OPAL_TOKEN::STARTLIST);
      cmd->addToken(OPAL_TOKEN::STARTLIST);
      cmd->addToken(OPAL_TOKEN::STARTNAME);
      cmd->addToken("Enabled");
      cmd->addToken(OPAL_TRUE);
      cmd->addToken(OPAL_TOKEN::ENDNAME);
      cmd->addToken(OPAL_TOKEN::ENDLIST);
      cmd->addToken(OPAL_TOKEN::ENDLIST);
      cmd->addToken(OPAL_TOKEN::ENDLIST);
      cmd->complete();

      // create session to use with erasemaster
      session = new DtaSession(this);
      if (session == NULL) {
        LOG(E) << "Unable to create session object ";
        delete cmd;
        return DTAERROR_OBJECT_CREATE_FAILED;
      }

      // MSID ?
      if (useMSID) session->dontHashPwd();

      // start session
      lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, (char *)pwd.c_str(), erasemaster);
      if (lastRC == 0)
        {
          // send command
          lastRC = session->sendCommand(cmd, response);
        }

      delete cmd;
      delete session;
    }

  return lastRC;
}

uint8_t DtaDevEnterprise::initLSPUsers(char * defaultPassword, char * newPassword)
{
  vector<uint8_t> user, usercpin, hash, erasemaster, table;
  uint8_t lastRC;
  LOG(D1) << "Entering DtaDevEnterprise::initLSPUsers()";

  // do erasemaster
  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  session->dontHashPwd();
  set8(erasemaster, OPALUID[OPAL_UID::ENTERPRISE_ERASEMASTER_UID]);
  if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, defaultPassword, erasemaster)) != 0) {
    delete session;
    return lastRC;
  }
  DtaHashPwd(hash, newPassword, this);
  user2cpin(usercpin, erasemaster);
  if ((lastRC = setTable(usercpin, "PIN", hash)) != 0) {
    LOG(E) << "Unable to set new EraseMaster password ";
    delete session;
    return lastRC;
  }
  LOG(D) << "EraseMaster  password set";
  delete session;
  // look up MaxRanges
  uint16_t MaxRanges = 0;
  if ((lastRC = getMaxRanges(&MaxRanges)) != 0) {
    return lastRC;
  }
  if (MaxRanges == 0 || MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  LOG(I) << "Maximum ranges supported " << MaxRanges;
  // do bandmasters
  set8(user, OPALUID[ENTERPRISE_BANDMASTER0_UID]);
  for (uint16_t i = 0; i <= MaxRanges; i++) {
    setband(user, i);
    LOG(D3) << "initializing BandMaster" << (uint16_t) i;
    session = new DtaSession(this);
    if (session == NULL) {
      LOG(E) << "Unable to create session object ";
      return DTAERROR_OBJECT_CREATE_FAILED;
    }
    session->dontHashPwd();
    if ((lastRC = session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, defaultPassword, user)) != 0) {
      delete session;
      // We only return failure if we fail to set BandMaster0.
      if (i == 0) {
        return lastRC;
      } else {
        continue;
      }
    }
    DtaHashPwd(hash, newPassword, this);
    user2cpin(usercpin, user);
    if ((lastRC = setTable(usercpin, "PIN", hash)) != 0) {
      LOG(E) << "Unable to set BandMaster" << (uint16_t) i << " new password ";
      // We only return failure if we fail to set BandMaster0.
      if (i == 0) {
        delete session;
        return lastRC;
      }
    } else {
      LOG(I) << "BandMaster" << (uint16_t) i << " password set";
    }
    delete session;
  }
  LOG(D1) << "Exiting DtaDevEnterprise::initLSPUsers()";
  return 0;
}

uint8_t DtaDevEnterprise::initLSPUsers(char * defaultPassword, vector<uint8_t> HostChallenge)
{
  uint8_t lastRC;
  LOG(D1) << "Entering DtaDevEnterprise::initLSPUsers()";

  // do erasemaster
  vector<uint8_t> erasemaster, usercpin;
  set8(erasemaster, OPALUID[OPAL_UID::ENTERPRISE_ERASEMASTER_UID]);
  user2cpin(usercpin, erasemaster);

  lastRC = WithSession([this, defaultPassword, erasemaster](){
    session->dontHashPwd();
    return session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, defaultPassword, erasemaster);
  },
    [this, usercpin, HostChallenge](){
      return setTable(usercpin, "PIN", HostChallenge);
    });

  if (lastRC != 0) {
    LOG(E) << "Unable to set new EraseMaster host challenge ";
    return lastRC;
  }
  LOG(D) << "EraseMaster  host challenge set";


  // look up MaxRanges
  uint16_t MaxRanges = 0;
  if ((lastRC = getMaxRanges(&MaxRanges)) != 0) {
    return lastRC;
  }
  if (MaxRanges == 0 || MaxRanges >= 1024)
    return DTAERROR_UNSUPORTED_LOCKING_RANGE;

  LOG(I) << "Maximum ranges supported " << MaxRanges;
  // do bandmasters
  vector<uint8_t> user;
  set8(user, OPALUID[ENTERPRISE_BANDMASTER0_UID]);
  for (uint16_t i = 0; i <= MaxRanges; i++) {
    LOG(D3) << "initializing BandMaster" << (uint16_t) i;
    setband(user, i);
    user2cpin(usercpin, user);

    lastRC = WithSession([this, defaultPassword, user](){
      session->dontHashPwd();
      return session->start(OPAL_UID::ENTERPRISE_LOCKINGSP_UID, defaultPassword, user);
    },
      [this, usercpin, HostChallenge](){
        return setTable(usercpin, "PIN", HostChallenge);
      });

    if (lastRC != 0) {
      LOG(E) << "Unable to set BandMaster" << (uint16_t) i << " host challenge ";
      // We only return failure if we fail to set BandMaster0.
      if (i == 0) {
        return lastRC;
      }
    } else {
      LOG(I) << "BandMaster" << (uint16_t) i << " host challenge set";
    }
  }
  LOG(D1) << "Exiting DtaDevEnterprise::initLSPUsers()";
  return 0;
}

uint8_t DtaDevEnterprise::getDefaultPassword()
{
  LOG(D1) << "Entering DtaDevEnterprise::getDefaultPassword()";
  uint8_t lastRC;
  vector<uint8_t> hash;
  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start(OPAL_UID::OPAL_ADMINSP_UID)) != 0) {
    LOG(E) << "Unable to start Unauthenticated session " << dev;
    delete session;
    return lastRC;
  }
  vector<uint8_t> table;
  table. push_back(OPAL_SHORT_ATOM::BYTESTRING8);
  for (int i = 0; i < 8; i++) {
    table.push_back(OPALUID[OPAL_UID::OPAL_C_PIN_MSID][i]);
  }
  if ((lastRC = getTable(table, "PIN", "PIN")) != 0) {
    delete session;
    return lastRC;
  }
  delete session;
  LOG(D1) << "Exiting getDefaultPassword()";
  return 0;
}
uint8_t DtaDevEnterprise::printDefaultPassword()
{
  string defaultPassword;
  const uint8_t rc = getMSID(defaultPassword);
  if (rc) {
    LOG(E) << "unable to retrieve MSID";
    return rc;
  }
  fprintf(stdout, "MSID: %s\n", (char *)defaultPassword.c_str());
  return 0;
}
uint8_t DtaDevEnterprise::setSIDPassword(char * oldpassword, char * newpassword,
                                         uint8_t hasholdpwd, uint8_t hashnewpwd)
{
  LOG(D1) << "Entering DtaDevEnterprise::setSIDPassword()";
  uint8_t lastRC;

  vector<uint8_t> user;
  set8(user, OPALUID[OPAL_SID_UID]);

  vector<uint8_t> usercpin;
  set8(usercpin, OPALUID[OPAL_C_PIN_SID]);

  if (*oldpassword == '\0')
    {
      string defaultPassword;
      if ((lastRC = getMSID(defaultPassword)) != 0) {
        LOG(E) << "setPassword failed to retrieve MSID";
        return lastRC;
      }
      session = new DtaSession(this);
      if (session == NULL) {
        LOG(E) << "Unable to create session object ";
        return DTAERROR_OBJECT_CREATE_FAILED;
      }
      session->dontHashPwd();
      if ((lastRC = session->start(OPAL_UID::OPAL_ADMINSP_UID, (char *)defaultPassword.c_str(), user)) != 0) {
        delete session;
        return lastRC;
      }
    }
  else
    {
      session = new DtaSession(this);
      if (session == NULL) {
        LOG(E) << "Unable to create session object ";
        return DTAERROR_OBJECT_CREATE_FAILED;
      }
      session->dontHashPwd();
      if (!hasholdpwd) session->dontHashPwd();
      if ((lastRC = session->start(OPAL_UID::OPAL_ADMINSP_UID, oldpassword, user)) != 0) {
        delete session;
        return lastRC;
      }
    }
  vector<uint8_t> hash;
  if (hashnewpwd)
    {
      DtaHashPwd(hash, newpassword, this);
    }
  else
    {
      hash.push_back(0xd0);
      hash.push_back((uint8_t)strnlen(newpassword, 255));
      for (uint16_t i = 0; i < strnlen(newpassword, 255); i++)
        {
          hash.push_back(newpassword[i]);
        }
    }
  if ((lastRC = setTable(usercpin, "PIN", hash)) != 0) {
    LOG(E) << "Unable to set new SID password ";
    delete session;
    return lastRC;
  }
  delete session;
  LOG(D1) << "Exiting DtaDevEnterprise::setSIDHostChallenge()";
  return 0;
}

uint8_t DtaDevEnterprise::setSIDHostChallenge(vector<uint8_t> oldHostChallenge,
                                              vector<uint8_t> newHostChallenge)
{
  LOG(D1) << "Entering DtaDevEnterprise::setSIDHostChallenge()";
  vector<uint8_t> user;
  set8(user, OPALUID[OPAL_SID_UID]);

  vector<uint8_t> usercpin;
  set8(usercpin, OPALUID[OPAL_C_PIN_SID]);

  uint8_t lastRC = WithSession([this, oldHostChallenge, user](){
    session->dontHashPwd();
    if (oldHostChallenge.size() == 0)
      {
        string defaultPassword;
        uint8_t rc = getMSID(defaultPassword);
        if (rc != 0) {
          LOG(E) << "setPassword failed to retrieve MSID";
          return rc;
        }
        return session->start(OPAL_UID::OPAL_ADMINSP_UID, (char *)defaultPassword.c_str(), user);
      }
    else
      {
        return session->start(OPAL_UID::OPAL_ADMINSP_UID, oldHostChallenge, user);
      }
  },
    [this, usercpin, newHostChallenge](){
      return setTable(usercpin, "PIN", newHostChallenge);
    });

  if (lastRC != 0) {
    LOG(E) << "Unable to set new SID password ";
  } else {
    LOG(D1) << "Exiting DtaDevEnterprise::setSIDHostChallenge()";
  }
  return lastRC;
}

uint8_t DtaDevEnterprise::setTable(vector<uint8_t> table, const char *name,
                                   OPAL_TOKEN value)
{
  vector <uint8_t> token;
  token.push_back((uint8_t) value);
  return(setTable(table, name, token));
}
uint8_t DtaDevEnterprise::setTable(vector<uint8_t> table, const char *name,
                                   vector<uint8_t> value)
{
  LOG(D1) << "Entering DtaDevEnterprise::setTable";
  uint8_t lastRC;
  DtaCommand *set = new DtaCommand();
  if (set == NULL) {
    LOG(E) << "Unable to create command object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  set->reset(OPAL_UID::OPAL_AUTHORITY_TABLE, OPAL_METHOD::ESET);
  set->changeInvokingUid(table);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTLIST);
  set->addToken(OPAL_TOKEN::STARTNAME);
  set->addToken(name);
  set->addToken(value);
  set->addToken(OPAL_TOKEN::ENDNAME);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->addToken(OPAL_TOKEN::ENDLIST);
  set->complete();
  if ((lastRC = session->sendCommand(set, response)) != 0) {
    LOG(E) << "Set Failed ";
    delete set;
    return lastRC;
  }
  delete set;
  LOG(D1) << "Leaving DtaDevEnterprise::setTable";
  return 0;
}
uint8_t DtaDevEnterprise::getTable(vector<uint8_t> table, const char * startcol,
                                   const char * endcol)
{
  LOG(D1) << "Entering DtaDevEnterprise::getTable";
  uint8_t lastRC;
  DtaCommand *get = new DtaCommand();
  if (get == NULL) {
    LOG(E) << "Unable to create command object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  get->reset(OPAL_UID::OPAL_AUTHORITY_TABLE, OPAL_METHOD::EGET);
  get->changeInvokingUid(table);
  get->addToken(OPAL_TOKEN::STARTLIST);
  get->addToken(OPAL_TOKEN::STARTLIST);
  get->addToken(OPAL_TOKEN::STARTNAME);
  get->addToken("startColumn");
  get->addToken(startcol);
  get->addToken(OPAL_TOKEN::ENDNAME);
  get->addToken(OPAL_TOKEN::STARTNAME);
  get->addToken("endColumn");
  get->addToken(endcol);
  get->addToken(OPAL_TOKEN::ENDNAME);
  get->addToken(OPAL_TOKEN::ENDLIST);
  get->addToken(OPAL_TOKEN::ENDLIST);
  get->complete();
  if ((lastRC = session->sendCommand(get, response)) != 0) {
    delete get;
    return lastRC;
  }
  delete get;
  return 0;
}
uint16_t DtaDevEnterprise::comID()
{
  LOG(D1) << "Entering DtaDevEnterprise::comID()";
  return device_info.Enterprise_basecomID;
}
uint8_t DtaDevEnterprise::exec(DtaCommand * cmd, DtaResponse & resp, uint8_t protocol)
{
  uint8_t rc = 0;
  DTA_Header * hdr = (DTA_Header *) cmd->getCmdBuffer();
  LOG(D1) << "Entering DtaDevEnterprize::exec";
  LOG(D3) << endl << "Dumping command buffer";
  IFLOG(D) DtaAnnotatedDump(TRUSTED_SEND, cmd->getCmdBuffer(), cmd->outputBufferSize());
  IFLOG(D3) DtaHexDump(cmd->getCmdBuffer(), SWAP32(hdr->cp.length) + sizeof (DTA_ComPacketHeader));
  rc = sendCmd(TRUSTED_SEND, protocol, comID(), cmd->getCmdBuffer(), cmd->outputBufferSize());
  if (0 != rc) {
    LOG(E) << "DtaDevEnterprize::exec Command failed on send " << (uint16_t) rc;
    return rc;
  }
  hdr = (DTA_Header *) cmd->getRespBuffer();
  do {
    //LOG(D) << "read loop";
    osmsSleep(25);
    memset(cmd->getRespBuffer(), 0, MIN_BUFFER_LENGTH);
    rc = sendCmd(TRUSTED_RECEIVE, protocol, comID(), cmd->getRespBuffer(), MIN_BUFFER_LENGTH);

  }
  while ((0 != hdr->cp.outstandingData) && (0 == hdr->cp.minTransfer));
  LOG(D3) << std::endl << "Dumping reply buffer";
  IFLOG(D) DtaAnnotatedDump(TRUSTED_RECEIVE, cmd->getRespBuffer(), SWAP32(hdr->cp.length) + sizeof (DTA_ComPacketHeader));
  IFLOG(D3) DtaHexDump(cmd->getRespBuffer(), SWAP32(hdr->cp.length) + sizeof (DTA_ComPacketHeader));
  if (0 != rc) {
    LOG(E) << "Command failed on recv, returned " << (uint16_t) rc;
    return rc;
  }
  resp.init(cmd->getRespBuffer());
  return 0;
}
uint8_t DtaDevEnterprise::properties()
{
  LOG(D1) << "Entering DtaDevEnterprise::properties()";
  uint8_t lastRC;
  session = new DtaSession(this);  // use the session IO without starting a session
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  DtaCommand *props = new DtaCommand(OPAL_UID::OPAL_SMUID_UID, OPAL_METHOD::PROPERTIES);
  if (props == NULL) {
    LOG(E) << "Unable to create command object ";
    delete session;
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  props->addToken(OPAL_TOKEN::STARTLIST);
  props->addToken(OPAL_TOKEN::STARTNAME);
  props->addToken("HostProperties");
  props->addToken(OPAL_TOKEN::STARTLIST);
  props->addToken(OPAL_TOKEN::STARTNAME);
  props->addToken("MaxComPacketSize");
  props->addToken(2048);
  props->addToken(OPAL_TOKEN::ENDNAME);
  props->addToken(OPAL_TOKEN::STARTNAME);
  props->addToken("MaxPacketSize");
  props->addToken(2028);
  props->addToken(OPAL_TOKEN::ENDNAME);
  props->addToken(OPAL_TOKEN::STARTNAME);
  props->addToken("MaxIndTokenSize");
  props->addToken(1992);
  props->addToken(OPAL_TOKEN::ENDNAME);
  props->addToken(OPAL_TOKEN::STARTNAME);
  props->addToken("MaxPackets");
  props->addToken(1);
  props->addToken(OPAL_TOKEN::ENDNAME);
  props->addToken(OPAL_TOKEN::STARTNAME);
  props->addToken("MaxSubpackets");
  props->addToken(1);
  props->addToken(OPAL_TOKEN::ENDNAME);
  props->addToken(OPAL_TOKEN::STARTNAME);
  props->addToken("MaxMethods");
  props->addToken(1);
  props->addToken(OPAL_TOKEN::ENDNAME);
  props->addToken(OPAL_TOKEN::ENDLIST);
  props->addToken(OPAL_TOKEN::ENDNAME);
  props->addToken(OPAL_TOKEN::ENDLIST);
  props->complete();
  if ((lastRC = session->sendCommand(props, propertiesResponse)) != 0) {
    delete props;
    return lastRC;
  }
  device_info.Properties = 1;
  delete props;
  LOG(D1) << "Leaving DtaDevEnterprise::properties()";
  return 0;
}
void DtaDevEnterprise::puke()
{

  LOG(D1) << "Entering DtaDevEnterprise::puke";
  DtaDev::puke();
  LOG(D1) << "DtaDevEnterprise::puke -- Enterprise-specific section";
  if (device_info.Properties) {

    uint32_t i = 0, j = 0;

    cout << std::endl << "TPer Properties: " << std::endl;
    for (i = 0, j = 1; i < propertiesResponse.getTokenCount(); i++) {
      if (OPAL_TOKEN::ENDLIST == propertiesResponse.tokenIs(i)) {
        if (OPAL_TOKEN::STARTNAME == propertiesResponse.tokenIs(i + 1)) {
          cout << std::endl << "Host Properties: " << std::endl;
          i += 2;
          j = 1;
          continue;
        }
        else {
          break;
        }
      }
      if (OPAL_TOKEN::STARTNAME == propertiesResponse.tokenIs(i)) {
        if (OPAL_TOKEN::DTA_TOKENID_BYTESTRING == propertiesResponse.tokenIs(i + 1)) {
          cout << "  " << propertiesResponse.getString(i + 1) << " = " << propertiesResponse.getUint64(i + 2);
          i += 2;
          j++;
          if (!(j % 3)) cout << std::endl;
        }
      }
    }
    if ((j % 3) != 0)
      cout << std::endl;
  }
  LOG(D1) << "Exiting DtaDevEnterprise::puke";
}

uint8_t DtaDevEnterprise::rawCmd(char *sp, char *hexauth, char *pass,
                                 char *hexinvokingUID, char *hexmethod,char *hexparms)
{
  LOG(D1) << "Entering DtaDevEnterprise::rawCmd";
  LOG(D1) << sp << " " << hexauth << " " << pass << " " ;
  LOG(D1) << hexinvokingUID << " " << hexmethod << " " << hexparms;
  uint8_t lastRC;
  vector<uint8_t> authority, object, invokingUID, method, parms;
  uint8_t work;
  if (16 != strnlen(hexauth, 32)) {
    LOG(E) << "Authority must be 16 byte ascii string of hex authority uid";
    return DTAERROR_INVALID_PARAMETER;
  }
  authority.push_back(OPAL_SHORT_ATOM::BYTESTRING8);
  for (uint32_t i = 0; i < 16; i += 2) {
    work = hexauth[i] & 0x40 ? 16 * ((hexauth[i] & 0xf) + 9) : 16 * (hexauth[i] & 0x0f);
    work += hexauth[i + 1] & 0x40 ? (hexauth[i + 1] & 0xf) + 9 : hexauth[i + 1] & 0x0f;
    authority.push_back(work);
  }
  if (16 != strnlen(hexinvokingUID, 32)) {
    LOG(E) << "invoker must be 16 byte ascii string of invoking uid";
    return DTAERROR_INVALID_PARAMETER;
  }
  invokingUID.push_back(OPAL_SHORT_ATOM::BYTESTRING8);
  for (uint32_t i = 0; i < 16; i += 2) {
    work = hexinvokingUID[i] & 0x40 ? 16 * ((hexinvokingUID[i] & 0xf) + 9) : 16 * (hexinvokingUID[i] & 0x0f);
    work += hexinvokingUID[i + 1] & 0x40 ? (hexinvokingUID[i + 1] & 0xf) + 9 : hexinvokingUID[i + 1] & 0x0f;
    invokingUID.push_back(work);
  }
  if (16 != strnlen(hexmethod, 32)) {
    LOG(E) << "invoker must be 16 byte ascii string of method uid";
    return DTAERROR_INVALID_PARAMETER;
  }
  method.push_back(OPAL_SHORT_ATOM::BYTESTRING8);
  for (uint32_t i = 0; i < 16; i += 2) {
    work = hexmethod[i] & 0x40 ? 16 * ((hexmethod[i] & 0xf) + 9) : 16 * (hexmethod[i] & 0x0f);
    work += hexmethod[i + 1] & 0x40 ? (hexmethod[i + 1] & 0xf) + 9 : hexmethod[i + 1] & 0x0f;
    method.push_back(work);
  }
  if (1020 < strnlen(hexparms, 1024)) {
    LOG(E) << "Parmlist limited to 1020 characters";
    return DTAERROR_INVALID_PARAMETER;
  }
  if (strnlen(hexparms, 1024) % 2) {
    LOG(E) << "Parmlist must be even number of bytes";
    return DTAERROR_INVALID_PARAMETER;
  }

  for (uint32_t i = 0; i < strnlen(hexparms, 1024); i += 2) {
    work = hexparms[i] & 0x40 ? 16 * ((hexparms[i] & 0xf) + 9) : 16 * (hexparms[i] & 0x0f);
    work += hexparms[i + 1] & 0x40 ? (hexparms[i + 1] & 0xf) + 9 : hexparms[i + 1] & 0x0f;
    parms.push_back(work);
  }
  DtaCommand *cmd = new DtaCommand();
  if (cmd == NULL) {
    LOG(E) << "Unable to create command object ";
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  cmd->reset(OPAL_UID::OPAL_AUTHORITY_TABLE, method);
  cmd->changeInvokingUid(invokingUID);
  cmd->addToken(parms);
  cmd->complete();
  session = new DtaSession(this);
  if (session == NULL) {
    LOG(E) << "Unable to create session object ";
    delete cmd;
    return DTAERROR_OBJECT_CREATE_FAILED;
  }
  if ((lastRC = session->start((OPAL_UID) atoi(sp), pass, authority)) != 0) {
    delete cmd;
    delete session;
    return lastRC;
  }
  LOG(D) << "Command:";
  cmd->dumpCommand();
  if ((lastRC = session->sendCommand(cmd, response)) != 0) {
    delete cmd;
    delete session;
    return lastRC;
  }
  LOG(D) << "Response:";
  cmd->dumpResponse();
  delete cmd;
  delete session;
  LOG(D1) << "Exiting DtaDevEnterprise::rawCmd";
  return 0;
}
uint8_t DtaDevEnterprise::objDump(char *sp, char * auth, char *pass,
                                  char * objID)
{
  LOG(D1) << "Entering DtaDevEnterprise::objDump";
  LOG(D1) << sp << " " << auth << " " << pass << " " << objID;
  if (16 != strnlen(auth, 32)) {
    LOG(E) << "Authority must be 16 byte ascii string of hex authority uid";
    return 0xff;
  }
  if (16 != strnlen(objID, 32)) {
    LOG(E) << "ObjectID must be 16 byte ascii string of hex object uid";
    return 0xff;
  }
  DtaCommand *get = new DtaCommand();
  vector<uint8_t> authority, object;
  uint8_t work;
  authority.push_back(OPAL_SHORT_ATOM::BYTESTRING8);
  for (uint32_t i = 0; i < 16; i += 2) {
    work = auth[i] & 0x40 ? 16 * ((auth[i] & 0xf) + 9) : 16 * (auth[i] & 0x0f);
    work += auth[i + 1] & 0x40 ? (auth[i + 1] & 0xf) + 9 : auth[i + 1] & 0x0f;
    authority.push_back(work);
  }
  object.push_back(OPAL_SHORT_ATOM::BYTESTRING8);
  for (uint32_t i = 0; i < 16; i += 2) {
    work = objID[i] & 0x40 ? 16 * ((objID[i] & 0xf) + 9) : 16 * (objID[i] & 0x0f);
    work += objID[i + 1] & 0x40 ? (objID[i + 1] & 0xf) + 9 : objID[i + 1] & 0x0f;
    object.push_back(work);
  }
  get->reset(OPAL_UID::OPAL_AUTHORITY_TABLE, OPAL_METHOD::EGET);
  get->changeInvokingUid(object);
  get->addToken(OPAL_TOKEN::STARTLIST);
  get->addToken(OPAL_TOKEN::STARTLIST);
  get->addToken(OPAL_TOKEN::ENDLIST);
  get->addToken(OPAL_TOKEN::ENDLIST);
  get->complete();
  LOG(D) << "Command:";
  get->dumpCommand();
  session = new DtaSession(this);
  if (session->start((OPAL_UID)atoi(sp), pass, authority)) {
    delete get;
    delete session;
    return 0xff;
  }
  if (session->sendCommand(get, response)) {
    delete get;
    delete session;
    return 0xff;
  }
  LOG(D) << "Response:";
  get->dumpResponse();
  delete get;
  delete session;
  LOG(D1) << "Exiting DtaDevEnterprise::objDump";
  return 0;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif


uint8_t DtaDevEnterprise::getMSID(string& MSID) {
  const uint8_t rc = getDefaultPassword();
  if (rc) {
    LOG(E) << "unable to read MSID password";
    return rc;
  }
  MSID = response.getString(5);
  LOG(D1) << "MSID=" << MSID;
  return 0;
}
