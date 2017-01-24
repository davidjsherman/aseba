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

#include "../../common/productids.h"
#include "../../common/consts.h"
#include "../../common/utils/utils.h"
#include "../../common/zeroconf/zeroconf.h"

int main(int argc, char* argv[])
{
	std::vector<std::string> targets;

	int argCounter = 1;
	while (argCounter < argc)
	{
		auto arg = argv[argCounter++];
		if (strncmp(arg, "-", 1) != 0)
			targets.push_back(arg);
	}

	Aseba::Zeroconf zs;
}
