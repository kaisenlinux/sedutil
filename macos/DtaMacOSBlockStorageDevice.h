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
#include "DtaBlockStorageDevice.h"
#include <string>
#include <map>

typedef std::map<std::string,std::string>dictionary;
typedef std::map<std::string,std::string>::iterator dictionary_iterator;

/** MacOS specific implementation SCSI generic ioctls to send commands to the
 * device
 */
class DtaMacOSBlockStorageDevice: public DtaBlockStorageDevice {
public:
    using DtaBlockStorageDevice::DtaBlockStorageDevice;
    /** Factory function to look at the devref and create an instance of
     *  DtaDevMacOSBlockStorageDevice itself (for unrecognized drives)
     *
     * @param devref OS device reference e.g. "/dev/sda"
     * @param disk_info reference to DTA_DEVICE_INFO structure filled out during device identification
     */
    static
    DtaMacOSBlockStorageDevice *
    getDtaMacOSBlockStorageDevice(const char * devref,
                                  DTA_DEVICE_INFO & disk_info);

    
  virtual uint8_t discovery0(DTA_DEVICE_INFO & di);

  ~DtaMacOSBlockStorageDevice(){}


  static
  bool identifyUsingSCSIInquiry(io_connect_t connection,
                                InterfaceDeviceID & interfaceDeviceIdentification,
                                DTA_DEVICE_INFO & disk_info);


    static bool identify(OSDEVICEHANDLE osDeviceHandle,
                         InterfaceDeviceID & interfaceDeviceIdentification,
                         DTA_DEVICE_INFO & device_info);


    // Get info from I/O Registry
    static
    bool BlockStorageDeviceUpdate(io_registry_entry_t deviceService, DTA_DEVICE_INFO & disk_info);

};


CFDictionaryRef createIOBlockStorageDeviceProperties(io_service_t deviceService);
