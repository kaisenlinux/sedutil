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

#include "os.h"

#include <cstdint>
#include <cstring>
#include <algorithm>

   // The next four lines are from
   // https://github.com/microsoft/Windows-driver-samples/blob/main/storage/tools/spti/src/spti.c
    
#include <devioctl.h>
#include <ntdddisk.h>
#include <ntddscsi.h>

#include "DtaEndianFixup.h"
#include "DtaHexDump.h"
#include "DtaDevWindowsDrive.h"
#include "ParseDiscovery0Features.h"

OSDEVICEHANDLE DtaDevWindowsDrive::fdopen(const char * devref)
{
  OSDEVICEHANDLE osDeviceHandle = openDeviceHandle(devref);

  if (INVALID_HANDLE_VALUE == (HANDLE)osDeviceHandle) {
      DWORD err = GetLastError();
      // This is a D1 because diskscan looks for open fail to end scan
      LOG(D1) << "Error opening device " << devref << " Error " << err;
      if (ERROR_ACCESS_DENIED == err) {
          LOG(E) << "You do not have proper authority to access the raw disk";
          LOG(E) << "Try running as Administrator";
      }
  }
  return osDeviceHandle;
}

void DtaDevWindowsDrive::fdclose()
{
  if (isOpen()) {
    closeDeviceHandle(osDeviceHandle);
  }
}

OSDEVICEHANDLE DtaDevWindowsDrive::openDeviceHandle(const char* devref)
{
    LOG(D4) << "Opening device handle for " << devref;
    OSDEVICEHANDLE osDeviceHandle = (OSDEVICEHANDLE) CreateFile(
        devref,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (INVALID_HANDLE_VALUE != osDeviceHandle)
        LOG(D4) << "Opened device handle " << HEXON(2) << (size_t)osDeviceHandle << " for " << devref;
    else
        LOG(D4) << "Failed to open device handle for " << devref;
    return osDeviceHandle;
}

void DtaDevWindowsDrive::closeDeviceHandle(OSDEVICEHANDLE osDeviceHandle) {
    LOG(D4) << "Closing device handle " << HEXON(2) << (size_t)osDeviceHandle;
    (void)CloseHandle((HANDLE)osDeviceHandle);
    LOG(D4) << "Closed device handle";
}


std::vector<std::string> DtaDevWindowsDrive::enumerateDtaDevWindowsDriveDevRefs()
{
    std::vector<std::string> devices;

    for (int i = 0; i < MAX_DISKS; i++) {
        char devref[261];
        sprintf_s(devref, sizeof(devref), "\\\\.\\PhysicalDrive%i", i);
        if (isDtaDevOSDriveDevRef(devref))
            devices.push_back(std::string(devref));
    }

    return devices;
}

uint8_t DtaDevWindowsDrive::discovery0(DTA_DEVICE_INFO& disk_info) {
    void* d0Response = alloc_aligned_MIN_BUFFER_LENGTH_buffer();
    if (d0Response == NULL)
        return DTAERROR_COMMAND_ERROR;
    memset(d0Response, 0, MIN_BUFFER_LENGTH);

    int lastRC = sendCmd(IF_RECV, 0x01, 0x0001, d0Response, MIN_BUFFER_LENGTH);
    if ((lastRC) != 0) {
        LOG(D4) << "Acquiring Discovery 0 response failed " << lastRC;
        return DTAERROR_COMMAND_ERROR;
    }
    parseDiscovery0Features((uint8_t*)d0Response, disk_info);
    free_aligned_MIN_BUFFER_LENGTH_buffer(d0Response);
    return DTAERROR_SUCCESS;
}
