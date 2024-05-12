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

#include <cstdint>
#include <cstring>
#include <algorithm>
#include "dirent.h"
#include "os.h"
#include "log.h"
#include "DtaEndianFixup.h"
#include "DtaHexDump.h"
#include "DtaDevLinuxDrive.h"

#include "ParseDiscovery0Features.h"


int DtaDevLinuxDrive::fdopen(const char * devref)
{
  if (access(devref, R_OK | W_OK)) {
    LOG(E) << "You do not have permission to access the raw device in write mode";
    LOG(E) << "Perhaps you might try sudo to run as root";
  }

  OSDEVICEHANDLE osDeviceHandle = openDeviceHandle(devref);

  if (osDeviceHandle < 0) {
    LOG(E) << "Error opening device " << devref << " " << (int32_t) osDeviceHandle;
    //        if (-EPERM == (int)osDeviceHandle) {
    //            LOG(E) << "You do not have permission to access the raw disk in write mode";
    //            LOG(E) << "Perhaps you might try sudo to run as root";
    //        }
  }
  return osDeviceHandle;
}

void DtaDevLinuxDrive::fdclose()
{
  if (0 <= (int)osDeviceHandle) {
    LOG(D4) << "Closing device file handle " << (int32_t) osDeviceHandle;
    closeDeviceHandle(osDeviceHandle);
  }
}


OSDEVICEHANDLE DtaDevLinuxDrive::openDeviceHandle(const char* devref) {
    return open(devref, O_RDWR);
}

void DtaDevLinuxDrive::closeDeviceHandle(OSDEVICEHANDLE osDeviceHandle) {
    close(osDeviceHandle);
}

std::vector<std::string> DtaDevLinuxDrive::enumerateDtaDevLinuxDriveDevRefs()
{
    std::vector<std::string> devices;

  DIR *dir = opendir("/dev");
  if (dir==NULL) {
    LOG(E) << "Can't read /dev ?!";
    return devices;
  }

  struct dirent *dirent;
  while (NULL != (dirent=readdir(dir))) {
    char devref[261];
    snprintf(devref,sizeof(devref),"/dev/%s",dirent->d_name);
    if (isDtaDevOSDriveDevRef(devref))
      devices.push_back(std::string(devref));
  }

  closedir(dir);

  std::sort(devices.begin(),devices.end());

  return devices;
}

uint8_t DtaDevLinuxDrive::discovery0(DTA_DEVICE_INFO & disk_info) {
  void * d0Response = alloc_aligned_MIN_BUFFER_LENGTH_buffer();
  if (d0Response == NULL)
      return DTAERROR_COMMAND_ERROR;
  memset(d0Response, 0, MIN_BUFFER_LENGTH);

  int lastRC = sendCmd(IF_RECV, 0x01, 0x0001, d0Response, MIN_BUFFER_LENGTH);
  if ((lastRC ) != 0) {
    LOG(D4) << "Acquiring Discovery 0 response failed " << lastRC;
    return DTAERROR_COMMAND_ERROR;
  }
  parseDiscovery0Features((uint8_t *)d0Response, disk_info);
  free_aligned_MIN_BUFFER_LENGTH_buffer(d0Response);
  return DTAERROR_SUCCESS;
}
