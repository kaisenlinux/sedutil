/* C:B**************************************************************************
This software is © 2014 Bright Plaza Inc. <drivetrust@drivetrust.com>

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
/** Print a hexdump of an area of memory
 * @param address Starting address for the dump
 * @param length  Length of the area to be dumped
 */
#define NOHEXDUMP 0
// TODO: NOHEXDUMP 1
#if NOHEXDUMP
void DtaHexDump(void *, unsigned int) {}
#else
#define DtaHexDump(address,length) __DtaHexDump(#address,address,length)
void __DtaHexDump(const char * tag, void * address, unsigned int length);
#endif
