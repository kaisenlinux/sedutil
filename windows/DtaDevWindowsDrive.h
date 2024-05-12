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

#include "os.h"
#include "DtaDevOSDrive.h"
// #include <errno.h>
// #include <fcntl.h>

/** virtual implementation for a disk interface-generic disk drive
 */
class DtaDevWindowsDrive : public DtaDevOSDrive {
public:

    /** Factory function to look at the devref to filter out whether it could be a DtaDevWindowsDrive
     *
     * @param devref OS device reference e.g. "/dev/sda"
     */

    static bool isDtaDevWindowsDriveDevRef(const char* devref);

    /** Factory function to enumerate all the devrefs that pass the above filter
     *
     */
    static
        std::vector<std::string> enumerateDtaDevWindowsDriveDevRefs(void);

    /** Factory function to look at the devref and create an instance of the appropriate subclass of
     *  DtaDevWindowsDrive
     *
     * @param devref OS device reference e.g. "/dev/sda"
     * @param pdisk_info weak reference to DTA_DEVICE_INFO structure filled out during device identification
     */
    static DtaDevWindowsDrive* getDtaDevWindowsDrive(const char* devref,
        DTA_DEVICE_INFO& disk_info);


    // bool isOpen(void) { return 0 < fd && (fcntl(fd, F_GETFL) != -1 || errno != EBADF); }
    bool isOpen(void) { return osDeviceHandle != (OSDEVICEHANDLE)INVALID_HANDLE_VALUE; }

    uint8_t discovery0(DTA_DEVICE_INFO& disk_info);

    DtaDevWindowsDrive(OSDEVICEHANDLE _fd) :osDeviceHandle(_fd) {}

    virtual ~DtaDevWindowsDrive() { fdclose(); }

    static OSDEVICEHANDLE openDeviceHandle(const char* devref);
    static void closeDeviceHandle(OSDEVICEHANDLE osDeviceHandle);

protected:

    static OSDEVICEHANDLE fdopen(const char* devref);
    void fdclose(void);

public:  // *** TODO *** DEBUGGING *** this should just be protected
    OSDEVICEHANDLE osDeviceHandle = (OSDEVICEHANDLE)INVALID_HANDLE_VALUE; /**< Windows handle for the device  */

};
/** iomanip commands to hexdump a field */
#include <iomanip>
#define HEXON(x) "0x" << std::hex << std::setw(x) << std::setfill('0')
/** iomanip command to return to standard ascii output */
#define HEXOFF std::dec << std::setw(0) << std::setfill(' ')
