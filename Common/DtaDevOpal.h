/* C:B**************************************************************************
   This software is Copyright (c) 2014-2024 Bright Plaza Inc. <drivetrust@drivetrust.com>

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
#pragma once

#include "DtaStructures.h"
#include "DtaLexicon.h"
#include "DtaDev.h"


using namespace std;
/** Common code for OPAL SSCs.
 * most of the code that works for OPAL 2.0 also works for OPAL 1.0
 * that common code is implemented in this class
 */
class DtaDevOpal : public DtaDev {
public:
  ~DtaDevOpal();

  void set_prop(DtaCommand * props, uint16_t sz_MaxComPacketSize, uint16_t sz_MaxResponseComPacketSize, uint16_t sz_MaxPacketSize, uint16_t sz_MaxIndTokenSize);
  void fill_prop(uint8_t show); // show = TRUE, print property  ; show = 0; fill property variable but no print
  uint8_t properties();
  /** Send a command to the device and wait for the response
   * @param cmd the MswdCommand object containg the command
   * @param resp the DtaResonse object containing the response
   * @param protocol The security protocol number to use for the command
   */
  uint8_t exec(DtaCommand * cmd, DtaResponse & resp, uint8_t protocol = 0x01);
  /** return the communications ID to be used for sessions to this device */
  virtual uint16_t comID() = 0;
  /** Change the SID password from it's MSID default
   * @param newpassword  new password for SID
   */
  uint8_t takeOwnership(char * newpassword);

  /** Change the SID HostChallenge from its MSID default
   * @param HostChallenge  new HostChallenge for SID
   */
  uint8_t takeOwnership(vector<uint8_t> HostChallenge);

  /** retrieve the MSID password */
  uint8_t printDefaultPassword();
  /** retrieve a single row from a table
   * @param table the UID of the table
   * @param startcol the starting column of data requested
   * @param endcol the ending column of the data requested
   */
  uint8_t getTable(vector<uint8_t> table, uint16_t startcol,
                   uint16_t endcol);
  /** Set the SID password.
   * Requires special handling because password is not always hashed.
   * @param oldpassword  current SID password
   * @param newpassword  value password is to be changed to
   * @param hasholdpwd  is the old password to be hashed before being added to the bytestream
   * @param hashnewpwd  is the new password to be hashed before being added to the bytestream
   */
  uint8_t setSIDPassword(char * oldpassword, char * newpassword,
                         uint8_t hasholdpwd = 1, uint8_t hashnewpwd = 1);

  /** Set the SID host challenge.
   * @param oldHostChallenge  current SID host challenge
   * @param newHostChallenge  value host challenge is to be changed to
   * @note neither value is hashed
   */
  uint8_t setSIDHostChallenge(vector<uint8_t> oldHostChallenge,
                              vector<uint8_t> newHostChallenge);
  /** set a single column in an object table
   * @param table the UID of the table
   * @param name the column name to be set
   * @param value data to be stored the the column
   */
  uint8_t setTable(vector<uint8_t> table, OPAL_TOKEN name,
                   vector<uint8_t> value);
  /** set a single column in an object table
   * @param table the UID of the table
   * @param name the column name to be set
   * @param value data to be stored the the column
   */
  uint8_t setTable(vector<uint8_t> table, OPAL_TOKEN name,
                   OPAL_TOKEN value);

  /** Change state of the Locking SP to active.
   * Enables locking
   * @param password  current SID password
   */
  uint8_t activateLockingSP(char * password);

  /** Enable locking on the device
   * @param HostChallenge HostChallenge of the admin sp SID authority
   */
  uint8_t activateLockingSP(vector<uint8_t>HostChallenge);

  /** Change state of the Locking SP to active in Single User Mode.
   * Enables locking in Single User Mode
   * @param lockingrange  the locking range number to activate in SUM
   * @param password  current SID password
   */
  uint8_t activateLockingSP_SUM(uint8_t lockingrange, char * password);
  /** Erase a Single User Mode locking range by calling the drive's erase method
   * @param lockingrange The Locking Range to erase
   * @param password The administrator password for the drive
   */
  uint8_t eraseLockingRange_SUM(uint8_t lockingrange, char * password);
  /** Restore the state of the Locking SP to factory defaults.
   * Enables locking
   * @param password  current SID password
   * @param keep boolean keep the data (NOT FUNCTIONAL)
   */
  uint8_t revertLockingSP(char * password, uint8_t keep = 0);

  /** Reset the Locking SP to its factory default condition
   * ERASES ALL DATA!
   * @param HostChallenge of Administrative user
   * @param keep true false for noerase function NOT WWORKING
   */
  uint8_t revertLockingSP(vector<uint8_t> HostChallenge, uint8_t keep = 0);

  /** get the UID or CPIN ID of a user from their character name
   * @param userid  Character user name
   *  @param column UID or CPIN to be returned
   *  @param userData The UIS or CPIN of the USER
   */
  uint8_t getAuth4User(char * userid, uint8_t column, std::vector<uint8_t> &userData);
  /**  Enable a user in the Locking SP
   * @param password the password of the Locking SP administrative authority
   * @param userid Character name of the user to be enabled
   */
  uint8_t enableUser(uint8_t mbrstate, char * password, char * userid);

  uint8_t enableUserRead(uint8_t mbrstate, char * password, char * userid);

  /** enable a locking sp user.
   * @param state 0 or 1
   * @param HostChallenge HostChallenge of locking sp administrative authority
   * @param userid  the user to be enabled
   */
  uint8_t enableUser(uint8_t state, vector<uint8_t> HostChallenge, char * userid);

  /** Enable locking on the device
   * @param state 0 or 1
   * @param HostChallenge HostChallenge of the admin sp SID authority
   */
  uint8_t enableUserRead(uint8_t state, vector<uint8_t> HostChallenge, char * userid);






  uint8_t userAccessEnable(uint8_t mbrstate, OPAL_UID UID, char * userid);
  /**
   * @param state 0 or 1
   * @param Admin1Password Locking SP authority with access to flag
   */
  uint8_t setMBRDone(uint8_t state, char * Admin1Password);
  /** Primitive to set the MBRDone flag.
   * @param state 0 or 1
   * @param Admin1HostChallenge  host challenge -- unsalted password of the locking administrative authority
   */
  uint8_t setMBRDone(uint8_t state, std::vector<uint8_t> Admin1HostChallenge);



  uint8_t TCGreset(uint8_t mbrstate);



  uint8_t STACK_RESET();



  /** Primitive to set the MBREnable flag.
   * @param state 0 or 1
   * @param Admin1Password Locking SP authority with access to flag
   */
  uint8_t setMBREnable(uint8_t state, char * Admin1Password);

  /** Primitive to set the MBREnable flag.
   * @param state 0 or 1
   * @param Admin1HostChallenge  host challenge -- unsalted password of the locking administrative authority
   */
  uint8_t setMBREnable(uint8_t state, vector<uint8_t> Admin1HostChallenge);


  /** Set the password of a locking SP user.
   * @param password  current password
   * @param userid the userid whose password is to be changed
   * @param newpassword  value password is to be changed to
   */
  uint8_t setPassword(char * password, char * userid, char * newpassword, uint8_t idx=0);


  /** Set the host challenge of a locking SP user.
   *   Note that the version above of this method is called setPassword
   * @param currentHostChallenge  current host challenge
   * @param userid the userid whose host challenge is to be changed
   * @param newHostChallenge  value  host challenge is to be changed to
   */
  uint8_t setHostChallenge(vector<uint8_t> currentHostChallenge, char * userid, vector<uint8_t> newHostChallenge, uint8_t idx=0);


  /** Set the password of a locking SP user in Single User Mode.
   * @param password  current user password
   * @param userid the userid whose password is to be changed
   * @param newpassword  value password is to be changed to
   */
  uint8_t setNewPassword_SUM(char * password, char * userid, char * newpassword);

  /** User command to manipulate the state of a locking range.
   * RW|RO|LK are the supported states @see OPAL_LOCKINGSTATE
   * @param lockingrange locking range number
   * @param lockingstate desired locking state (see above)
   * @param Admin1Password password of the locking administrative authority
   */
  uint8_t setLockingRange(uint8_t lockingrange, uint8_t lockingstate, char * Admin1Password, uint8_t idx=0);

  /** User command to manipulate the state of a locking range.
   * RW|RO|LK are the supported states @see OPAL_LOCKINGSTATE
   * @param lockingrange locking range number
   * @param lockingstate desired locking state (see above)
   * @param Admin1HostChallenge  host challenge -- unsalted password of the locking administrative authority
   */
  uint8_t setLockingRange(uint8_t lockingrange, uint8_t lockingstate, vector<uint8_t> Admin1HostChallenge, uint8_t idx=0);

  /** Change the locking state of a locking range in Single User Mode
   * @param lockingrange The number of the locking range (0 = global)
   * @param lockingstate  the locking state to set
   * @param password password of user authority for the locking range
   */
  uint8_t setLockingRange_SUM(uint8_t lockingrange, uint8_t lockingstate, char * password);
  /** Setup a locking range.  Initialize a locking range, set it's start
   *  LBA and length, initialize it as unlocked with locking disabled.
   *  @paran lockingrange The Locking Range to be setup
   *  @param start  Starting LBA
   *  @param length Number of blocks
   *  @param password Password of administrator
   */
  uint8_t setupLockingRange(uint8_t lockingrange, uint64_t start,
                            uint64_t length, char * password);
  /** Setup a locking range in Single User Mode.  Initialize a locking range,
   *  set it's start LBA and length, initialize it as unlocked with locking enabled.
   *  @paran lockingrange The Locking Range to be setup
   *  @param start  Starting LBA
   *  @param length Number of blocks
   *  @paran password Password of administrator
   */
  uint8_t setupLockingRange_SUM(uint8_t lockingrange, uint64_t start,
                                uint64_t length, char * password);
  /** List status of locking ranges.
   *  @param password Password of administrator
   */
  uint8_t listLockingRanges(char * password, int16_t rangeid, uint8_t idx=0);
  /** User command to enable/disable a locking range.
   * RW|RO|LK are the supported states @see OPAL_LOCKINGSTATE
   * @param lockingrange locking range number
   * @param enabled boolean true = enabled, false = disabled
   * @param password password of the locking administrative authority
   */
  uint8_t configureLockingRange(uint8_t lockingrange, uint8_t enabled, char * password, uint8_t idx=0);

  /** User command to enable/disable a locking range.
   * RW|RO|LK are the supported states @see OPAL_LOCKINGSTATE
   * @param lockingrange locking range number
   * @param enabled boolean true = enabled, false = disabled
   * @param HostChallenge HostChallenge of the locking administrative authority
   */
  uint8_t configureLockingRange(uint8_t lockingrange, uint8_t enabled, vector<uint8_t> HostChallenge, uint8_t idx=0);


  /** Generate a new encryption key for a locking range.
   * @param lockingrange locking range number
   * @param password password of the locking administrative authority
   */
  uint8_t rekeyLockingRange(uint8_t lockingrange, char * password);
  /** Generate a new encryption key for a Single User Mode locking range.
   * @param LR locking range UID in vector format
   * @param UID user UID in vector format
   * @param password password of the UID authority
   */
  uint8_t rekeyLockingRange_SUM(vector<uint8_t> LR, vector<uint8_t>  UID, char * password);
  uint8_t setBandsEnabled(int16_t rangeid, char * password);
  /** Reset the TPER to its factory condition
   * ERASES ALL DATA!
   * @param password password of authority (SID or PSID)
   * @param PSID true or false is the authority the PSID
   *   */
  uint8_t revertTPer(char * password, uint8_t PSID = 0, uint8_t AdminSP = 0);

  /** Reset the TPER to its factory condition
   * @param HostChallenge HostChallenge of authority (SID or PSID)
   * @param PSID true or false is the authority the PSID
   * @param AdminSP true or false is the SP the AdminSP or ThisSP (Enterprise Only)
   */
  uint8_t revertTPer(vector<uint8_t> HostChallenge, uint8_t PSID = 0, uint8_t AdminSP = 0 );

  /** Erase a locking range
   * @param lockingrange The number of the locking range (0 = global)
   * @param password Password of administrative authority for locking range
   */
  uint8_t eraseLockingRange(uint8_t lockingrange, char * password);

  uint8_t activate(char * password);
  uint8_t getmfgstate(void);
  uint8_t loadPBA(char * password, char * filename);
  uint8_t setTperResetEnable(bool enable, char * password);
  uint8_t setLockonReset(uint8_t lockingrange, bool enable, char * password);
  uint8_t setLockonReset(uint8_t lockingrange, bool enable, vector<uint8_t>HostChallenge);
  /** User command to prepare the device for management by sedutil.
   * Specific to the SSC that the device supports
   * @param password the password that is to be assigned to the SSC master entities
   */
  uint8_t initialSetup(char * password);

  /** User command to prepare the device for management by sedutil.
   * Specific to the SSC that the device supports
   * @param HostChallenge the HostChallenge that is to be assigned to the SSC master entities
   */
  uint8_t initialSetup(vector<uint8_t>HostChallenge);

  /** User command to prepare the drive for Single User Mode and rekey a SUM locking range.
   * @param lockingrange locking range number to enable
   * @param start LBA to start locking range
   * @param length length (in blocks) for locking range
   * @param Admin1Password admin1 password for TPer
   * @param password User password to set for locking range
   */
  uint8_t setup_SUM(uint8_t lockingrange, uint64_t start, uint64_t length, char *Admin1Password, char * password);
  /** Displays the identify and discovery 0 information */
  void adj_host_prop(uint8_t act);
  void puke();

  /** Dumps an object for diagnostic purposes
   * @param sp index into the OPALUID table for the SP the object is in
   * @param auth the authority ti use for the dump
   * @param pass the password for the suthority
   * @param objID the UID of the object to dump
   *  */
  uint8_t objDump(char *sp, char * auth, char *pass,
                  char * objID);
  /** Issue any command to the drive for diagnostic purposes
   * @param sp index into the OPALUID table for the SP the object is in
   * @param auth the authority ti use for the dump
   * @param pass the password for the suthority
   * @param invoker caller of the method
   * @param method the method to call
   * @param plist  the parameter list for the command
   *
   */
  uint8_t rawCmd(char *sp, char * auth, char *pass,
                 char *invoker, char *method, char *plist);


  /** Primitive to extract the MSID into a std::string
   * @param MSID the string to receive the MSID
   */
  uint8_t getMSID(std::string& MSID);

  /** Short-circuit routine re-uses initialized drive and disk_info */
  DtaDevOpal(const char * devref, DtaDrive * drive, DTA_DEVICE_INFO& di) :
    DtaDev(devref, drive, di)
  {};


protected:
  /** Primitive to handle the setting of a value in the locking sp.
   * @param table_uid UID of the table
   * @param name column to be altered
   * @param value the value to be set
   * @param password password for the administrative authority
   * @param msg message to be displayed upon successful update;
   */
  uint8_t setLockingSPvalue(OPAL_UID table_uid, OPAL_TOKEN name, OPAL_TOKEN value,
                            char * password, char * msg = (char *) "New Value Set", uint8_t idx=0);

  /** Primitive to handle the setting of a value in the locking sp.
   * @param table_uid UID of the table
   * @param name column to be altered
   * @param value the value to be set
   * @param HostChallenge host challenge for the administrative authority
   * @param msg message to be displayed upon successful update;
   */
  uint8_t setLockingSPvalue(OPAL_UID table_uid, OPAL_TOKEN name, OPAL_TOKEN value,
                            vector<uint8_t>HostChallenge, char * msg = (char *) "New Value Set", uint8_t idx=0);

  uint8_t getDefaultPassword();
  uint8_t getTryLimit(uint16_t col1, uint16_t col2, char * pass);
  typedef struct lrStatus
  {
    uint8_t command_status; //return code of locking range query command
    uint8_t lockingrange_num; //which locking range is this
    uint64_t start;
    uint64_t size;
    bool RLKEna;
    bool WLKEna;
    bool RLocked;
    bool WLocked;
  }lrStatus_t;
  /** Get info programatically for single locking range
   *  @param lockingrange locking range number to check
   *  @param password Admin1 Password for TPer
   */
  lrStatus_t getLockingRange_status(uint8_t lockingrange, char * password);
  OPAL_UID getUSERUID(uint8_t idx) {
    return usermodeON ? (OPAL_UID)(OPAL_USER1_UID + idx) : OPAL_ADMIN1_UID;
  };
  uint8_t adminEnabledTab[16] = { 0 };
  uint8_t userEnabledTab[16] =  { 0 };
private:
  uint8_t __setLockingRange(uint8_t lockingrange, uint8_t lockingstate, std::function<uint8_t(void)>startSessionFn);
};
