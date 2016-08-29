/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
 Stephane Magnenat <stephane at magnenat dot net>
 (http://stephane.magnenat.net)
 and other contributors, see authors.txt for details

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../utils/utils.h"
#include "../utils/FormatableString.h"
#include <dashel/dashel.h>
#include "zeroconf.h"

using namespace std;

namespace Aseba
{
	//! A TXT record is composed of length-prefixed strings of the form KEY[=VALUE]
	string Zeroconf::TxtRecord::record() const
	{
		ostringstream txt;
		add(txt, "txtvers");
		add(txt, "protovers");
		add(txt, "type");
		add(txt, "ids");
		add(txt, "pids");
		return txt.str();
	}
	//! constructor for an Aseba target, with node ids and product ids
	Zeroconf::TxtRecord::TxtRecord(int protovers, const std::string& type, const std::vector<int>& ids, const std::vector<int>& pids)
	{
		assign("txtvers", 1);
		assign("protovers", protovers);
		assign("type", type);
		assign("ids", ids);
		assign("pids", pids);
	}
	void Zeroconf::TxtRecord::add(ostringstream& txt, const std::string& key) const
	{
		string record = key + "=" + fields.at(key);
		txt.put(record.length());
		txt << record;
	}
	//! a string value in the TXT record is a sequence of bytes
	void Zeroconf::TxtRecord::assign(const std::string& key, const std::string& value)
	{
		fields[key] = value.substr(0,20); // silently truncate name to 20 characters
	}
	//! a simple integer value in the TXT record is the string of its decimal value
	void Zeroconf::TxtRecord::assign(const std::string& key, const int value)
	{
		ostringstream field;
		field << value;
		assign(key, field.str());
	}
	//! a vector of integers in the TXT record is a sequence of big-endian 16-bit values
	//! note that the size of the vector is implicit in the record length: (record.length()-record.find("=")-1)/2.
	void Zeroconf::TxtRecord::assign(const std::string& key, const std::vector<int>& values)
	{
		ostringstream field;
		for (const auto &value : values)
			field.put(value<<8), field.put(value % 0xff);
		assign(key, field.str());
	}

} // namespace Aseba
