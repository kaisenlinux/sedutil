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

// This routine should be compiled with the main routine sedutil-cli and the
// appropriate Common/Customizations folder, and not with the sedutil library.

#include "os.h"
#include "log.h"
#include "DtaOptions.h"
#include "DtaLexicon.h"
#include "Version.h"

void usage()
{
    printf("sedutil v%s Copyright (c) 2014-2024 Bright Plaza Inc. <drivetrust@drivetrust.com>\n", GIT_VERSION);
    printf("a utility to manage self encrypting drives that conform\n");
    printf("to the Trusted Computing Group Storage Architecture Core Specification\n");
    printf("and to one of the Security Subsystem Class specifications such as OPAL 2.0\n");
    printf("General Usage:                     (see readme for extended commandset)\n");
    printf("sedutil-cli <-v> <-n> <action> <options> <device>\n");
    printf("-v (optional)                       increase verbosity, one to five v's\n");
    printf("-n (optional)                       no password hashing. Passwords will be sent in clear text!\n");
    printf("-l (optional)                       log style output to stderr only\n");
    printf("actions \n");
    printf("--scan\n");
    printf("                                Scans the devices on the system \n");
    printf("                                identifying the TPers \n");
    printf("--scan --json\n");
    printf("                                Scans the devices on the system \n");
    printf("                                identifying the TPers; output in JSON \n");
    printf("--scan --json-compact\n");
    printf("                                Scans the devices on the system \n");
    printf("                                identifying the TPers; output in compact JSON \n");
    printf("--query <device>\n");
    printf("                                Display the complete device information, including Discovery 0 response\n");
    printf("--query --json <device>\n");
    printf("                                Display the complete device information, including Discovery 0 response, in JSON format\n");
    printf("--query --json-compact <device>\n");
    printf("                                Display the complete device information, including the Discovery 0 response, in compact JSON format\n");
    printf("--isValidSED <device>\n");
    printf("                                Verify whether the given device is SED or not\n");
    printf("--listLockingRanges <password> <device>\n");
    printf("                                List all Locking Ranges\n");
    printf("--listLockingRange <0...n> <password> <device>\n");
    printf("                                List all Locking Ranges\n");
    printf("                                0 = GLobal 1..n  = LRn \n");
    printf("--rekeyLockingRange <0...n> <password> <device>\n");
    printf("                                0 = GLobal 1..n  = LRn \n");
    printf("                                Rekey Locking Range\n");
    printf("--setBandsEnabled <password> <device>\n");
    printf("                                Set Enabled for all Locking Ranges\n");
    printf("                                (password = \"\" for MSID) \n");
    printf("--setBandEnabled <0...n> <password> <device>\n");
    printf("                                Set Enabled for Locking Range[n]\n");
    printf("                                (password = \"\" for MSID) \n");
    printf("--eraseLockingRange <0...n> <password> <device>\n");
    printf("                                Erase a Locking Range\n");
    printf("                                0 = GLobal 1..n  = LRn \n");
    printf("--setupLockingRange <0...n> <RangeStart> <RangeLength> <password> <device>\n");
    printf("                                Setup a new Locking Range\n");
    printf("                                0 = GLobal 1..n  = LRn \n");
    printf("--initialSetup <SIDpassword> <device>\n");
    printf("                                Setup the device for use with sedutil\n");
    printf("                                <SIDpassword> is new SID and Admin1 password\n");
    printf("--setSIDPassword <SIDpassword> <newSIDpassword> <device> \n");
    printf("                                Change the SID password\n");
    printf("--setAdmin1Pwd <Admin1password> <newAdmin1password> <device> \n");
    printf("                                Change the Admin1 password\n");
    printf("--setPassword <oldpassword, \"\" for MSID> <userid> <newpassword> <device> \n");
    printf("                                Change the Enterprise password for userid\n");
    printf("                                \"EraseMaster\" or \"BandMaster<n>\", 0 <= n <= 1023\n");
    printf("--setLockingRange <0...n> <RW|RO|LK> <Admin1password> <device> \n");
    printf("                                Set the status of a Locking Range\n");
    printf("                                0 = GLobal 1..n  = LRn \n");
    printf("--enableLockingRange <0...n> <Admin1password> <device> \n");
    printf("                                Enable a Locking Range\n");
    printf("                                0 = GLobal 1..n  = LRn \n");
    printf("--disableLockingRange <0...n> <Admin1password> <device> \n");
    printf("                                Disable a Locking Range\n");
    printf("                                0 = GLobal 1..n  = LRn \n");
    printf("--setMBREnable <on|off> <Admin1password> <device> \n");
    printf("                                Enable|Disable MBR shadowing \n");
    printf("--setMBRDone <on|off> <Admin1password> <device> \n");
    printf("                                set|unset MBRDone\n");
    printf("--loadPBAimage <Admin1password> <file> <device> \n");
    printf("                                Write <file> to MBR Shadow area\n");
    printf("--revertTPer <SIDpassword> <device>\n");
    printf("                                set the device back to factory defaults \n");
    printf("                                This **ERASES ALL DATA** \n");
    printf("--revertNoErase <Admin1password> <device>\n");
    printf("                                deactivate the Locking SP \n");
    printf("                                without erasing the data \n");
    printf("                                on GLOBAL RANGE *ONLY* \n");
    printf("--yesIreallywanttoERASEALLmydatausingthePSID <PSID> <device>\n");
    printf("                                revert the device using the PSID *ERASING* *ALL* the data \n");
    printf("--printDefaultPassword <device>\n");
    printf("                                print MSID \n");

    // Customization commands
    printf("--version \n");
    printf("                                print sedutil-cli version, including build tag\n");

    printf("\n");
    printf("Examples \n");
    printf("sedutil-cli --scan <skipdevice>\n");
    printf("sedutil-cli --query %s \n", DEVICEEXAMPLE);
    printf("sedutil-cli --yesIreallywanttoERASEALLmydatausingthePSID <PSIDALLCAPSNODASHED> %s \n", DEVICEEXAMPLE);
    printf("sedutil-cli --initialSetup <newSIDpassword> %s \n", DEVICEEXAMPLE);

    // Customization examples
    printf("sedutil-cli --version \n");
    //    printf("--createUSB <file> <device1> <device2\n");
    //    printf("                                Write image file to USB\n");

    return;
}


#define LOCKINGRANGEARG(lockingrange) \
TESTARG(0, lockingrange, 0)            \
TESTARG(1, lockingrange, 1)            \
TESTARG(2, lockingrange, 2)            \
TESTARG(3, lockingrange, 3)            \
TESTARG(4, lockingrange, 4)            \
TESTARG(5, lockingrange, 5)            \
TESTARG(6, lockingrange, 6)            \
TESTARG(7, lockingrange, 7)            \
TESTARG(8, lockingrange, 8)            \
TESTARG(9, lockingrange, 9)            \
TESTARG(10, lockingrange, 10)          \
TESTARG(11, lockingrange, 11)          \
TESTARG(12, lockingrange, 12)          \
TESTARG(13, lockingrange, 13)          \
TESTARG(14, lockingrange, 14)          \
TESTARG(15, lockingrange, 15)          \
TESTFAIL("Invalid Locking Range (0-15)")

#define MBRSTATEARG(arg,mbrstate) \
TESTARG(ON, mbrstate, 1)      \
TESTARG(on, mbrstate, 1)      \
TESTARG(Y, mbrstate, 1)       \
TESTARG(y, mbrstate, 1)       \
TESTARG(n, mbrstate, 0)       \
TESTARG(N, mbrstate, 0)       \
TESTARG(off, mbrstate, 0)     \
TESTARG(OFF, mbrstate, 0)     \
TESTFAIL("Invalid " #arg "argument not <ON|on|Y|y|OFF|off|N|n>")

#define LOCKINGSTATEARG(lockingstate) \
TESTARG(RW, lockingstate, OPAL_LOCKINGSTATE::READWRITE) \
TESTARG(rw, lockingstate, OPAL_LOCKINGSTATE::READWRITE) \
TESTARG(RO, lockingstate, OPAL_LOCKINGSTATE::READONLY)  \
TESTARG(ro, lockingstate, OPAL_LOCKINGSTATE::READONLY)  \
TESTARG(LK, lockingstate, OPAL_LOCKINGSTATE::LOCKED)    \
TESTARG(lk, lockingstate, OPAL_LOCKINGSTATE::LOCKED)    \
TESTFAIL("Invalid locking state <RW|rw|RO|ro|LK|lk>")

#define TCGRESETTYPEARG(resettype) \
TESTARG(0, resettype, 0) \
TESTARG(1, resettype, 1) \
TESTARG(2, resettype, 2) \
TESTARG(3, resettype, 3) \
TESTFAIL("Invalid TCGreset argument not <0|1|2|3>")

uint8_t DtaOptions(int argc, char * argv[], DTA_OPTIONS * opts)
{
    memset(opts, 0, sizeof (DTA_OPTIONS));
    opts->output_format = DEFAULT_OUTPUT_FORMAT;
    uint16_t loggingLevel = DEFAULT_LOGGING_LEVEL;
    uint8_t baseOptions = 2; // program and option
    CLogLevel = CLog::FromInt(loggingLevel);
    RCLogLevel = RCLog::FromInt(loggingLevel);
    if (2 > argc) {
        usage();
        return DTAERROR_INVALID_COMMAND;
    }
    for (uint8_t i = 1; i < argc; i++) {
        if (!(strcmp("-h", argv[i])) || !(strcmp("--help", argv[i]))) {
            usage();
            return DTAERROR_INVALID_COMMAND;
        }
        
        if ('v' == argv[i][1]) {
            // logging level set to length of any arg staring with 'v'
          baseOptions += 1;
          loggingLevel += (uint16_t)(strlen(argv[i]) - 1);
          if (loggingLevel > MAX_LOGGING_LEVEL) loggingLevel = MAX_LOGGING_LEVEL;
          CLogLevel = CLog::FromInt(loggingLevel);
          RCLogLevel = RCLog::FromInt(loggingLevel);
          LOG(D) << "Log level set to " << CLog::ToString(CLog::FromInt(loggingLevel));
          LOG(D) << "sedutil version : " << GIT_VERSION;
        } else if (!(strcmp("-a", argv[i]))) {
          baseOptions += 1;
          opts->skip_activate = true;
          LOG(D) << "Do not activate LockingSP";
        } else if (!(strcmp("-u", argv[i]))) {
          baseOptions += 1;
          opts->usermode = true;
          LOG(D) << "user mode ON";
        } else if (!(strcmp("-t", argv[i]))) {
          baseOptions += 1;
          opts->translate_req = true;
          LOG(D) << "translate hashed string to data";
        } else if (!(strcmp("-n", argv[i]))) {
          baseOptions += 1;
          opts->no_hash_passwords = true;
          LOG(D) << "Password hashing is disabled";
        } else if (!strcmp("-l", argv[i])) {
          baseOptions += 1;
          opts->output_format = sedutilNormal;
          outputFormat = sedutilNormal;
        } else if (!(('-' == argv[i][0]) && ('-' == argv[i][1])) &&
                                            (0 == opts->action)) {
          LOG(E) << "Argument " << (uint16_t) i << " (" << argv[i] << ") should be a command";
          return DTAERROR_INVALID_COMMAND;
        }

        BEGIN_OPTION(initialSetup, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(setSIDPassword, 3) OPTION_IS(password) OPTION_IS(newpassword)
            OPTION_IS(device) END_OPTION
        BEGIN_OPTION(setup_SUM, 6)
	    LOCKINGRANGEARG(lockingrange)
            OPTION_IS(lrstart)
            OPTION_IS(lrlength)
            OPTION_IS(password)
            OPTION_IS(newpassword)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setAdmin1Pwd, 3)
            OPTION_IS(password)
            OPTION_IS(newpassword)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(activate, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(getmfgstate, 1) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(revertTPer, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(revertNoErase, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(PSIDrevert, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(PSIDrevertAdminSP, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(yesIreallywanttoERASEALLmydatausingthePSID, 2)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(enableuser, 4)
            MBRSTATEARG(enableuser,mbrstate)
            OPTION_IS(password)
            OPTION_IS(userid)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(enableuserread, 4)
            MBRSTATEARG(enableuser,mbrstate)
            OPTION_IS(password)
            OPTION_IS(userid)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(activateLockingSP, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(activateLockingSP_SUM, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(eraseLockingRange_SUM, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION_MAX(query, 2)
            TESTOPTIONALARG(--json, output_format, sedutilJSON)
            TESTOPTIONALARG(--json-compact, output_format, sedutilJSONCompact)
            ENDOPTIONALARGS
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION_MAX(scan, 1) 
            TESTOPTIONALARG(--json, output_format, sedutilJSON)
            TESTOPTIONALARG(--json-compact, output_format, sedutilJSONCompact)
            ENDOPTIONALARGS
        END_OPTION
        BEGIN_OPTION(version, 0)  END_OPTION
        BEGIN_OPTION(isValidSED, 1) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(eraseLockingRange, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(takeOwnership, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(revertLockingSP, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(setPassword, 4)
            OPTION_IS(password)
            OPTION_IS(userid)
            OPTION_IS(newpassword)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setPassword_SUM, 4)
            OPTION_IS(password)
            OPTION_IS(userid)
            OPTION_IS(newpassword)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(validatePBKDF2, 0) END_OPTION
        BEGIN_OPTION(setMBREnable, 3)
            MBRSTATEARG(MBREnable,mbrstate)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setMBRDone, 3)
            MBRSTATEARG(MBRDone,mbrstate)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(TCGreset, 2)
            TCGRESETTYPEARG(resettype)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setLockingRange, 4)
            LOCKINGRANGEARG(lockingrange)
            LOCKINGSTATEARG(lockingstate)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setLockingRange_SUM, 4)
            LOCKINGRANGEARG(lockingrange)
            LOCKINGSTATEARG(lockingstate)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(enableLockingRange, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(disableLockingRange, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setupLockingRange, 5)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(lrstart)
            OPTION_IS(lrlength)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setupLockingRange_SUM, 5)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(lrstart)
            OPTION_IS(lrlength)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(readonlyLockingRange, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(listLockingRanges, 2)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(listLockingRange, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(rekeyLockingRange, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setBandsEnabled, 2)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(setBandEnabled, 3)
            LOCKINGRANGEARG(lockingrange)
            OPTION_IS(password)
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(objDump, 5)
            i += 4;
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(printDefaultPassword, 1) OPTION_IS(device) END_OPTION
        BEGIN_OPTION(rawCmd, 7)
            i += 6;
            OPTION_IS(device)
        END_OPTION
        BEGIN_OPTION(hashvalidation, 2) OPTION_IS(password) OPTION_IS(device) END_OPTION

#include "Customizations/DtaExtensionOptions.inc"

        else {
            LOG(E) << "Invalid command line argument " << argv[i];
			return DTAERROR_INVALID_COMMAND;
        }
    }
    return DTAERROR_SUCCESS;
}
