/**************************************************************************/
/*  ip_wii.cpp                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                               HOMEBRODOT                               */
/**************************************************************************/
/* Copyright (c) 2023-present Homebrodot contributors.                    */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "ip_wii.h"

#include <network.h>

void IP_Wii::_resolve_hostname(List<IP_Address> &r_addresses, const String &p_hostname, Type p_type) const {
	if (p_type == IP::TYPE_IPV6) {
		ERR_PRINT("Wii does not support IPv6");
		return;
	}

	hostent *ent = net_gethostbyname(p_hostname.utf8().get_data());

	if (!ent || ent->h_length == 0) {
		ERR_PRINT("Failed to resolve \"" + p_hostname + "\"");
		return;
	}

	IP_Address ip;
	ip.set_ipv4(reinterpret_cast<const uint8_t *>(ent->h_addr_list[0]));
	r_addresses.push_back(ip);
}

void IP_Wii::get_local_interfaces(Map<String, Interface_Info> *r_interfaces) const {
	// TODO: what the fuck counts as a network interface

	Map<String, Interface_Info>::Element *E = r_interfaces->find("wii");
	if (!E) {
		Interface_Info info;
		info.name = "wii";
		info.name_friendly = "wii";
		info.index = "1";
		E = r_interfaces->insert("wii", info);
		ERR_FAIL_COND(!E);
	}

	Interface_Info &info = E->get();
	IP_Address ip;
	u32 ip_num = net_gethostip();
	ip.set_ipv4(reinterpret_cast<const uint8_t *>(ip_num));
	info.ip_addresses.push_front(ip);
}

IP *IP_Wii::_create_wii() {
	return memnew(IP_Wii);
}

void IP_Wii::make_default() {
	_create = _create_wii;
}

IP_Wii::IP_Wii() {
}
