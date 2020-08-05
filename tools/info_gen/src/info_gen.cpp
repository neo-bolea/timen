#include <assert.h>
#include <direct.h>
#include <fstream>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

struct proc_info
{
	int ProcID;
	float Time;
};

void main()
{
	assert(_chdir("../data") == 0);

	std::ifstream sym_file("Log.out");
	assert(!sym_file.fail());
	std::string cur_proc_str;
	std::vector<proc_info> proc_infos;

	do
	{
		int ProcID;
		float Time;

		sym_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		sym_file >> ProcID;
		sym_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		sym_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		sym_file >> Time;
		sym_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		sym_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		proc_infos.push_back(proc_info{ ProcID, Time });
	} while (!sym_file.eof());


}