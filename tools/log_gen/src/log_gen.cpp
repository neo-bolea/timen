#include <assert.h>
#include <direct.h>
#include <fstream>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

void main()
{
	assert(_chdir("..\\data") == 0);

	std::ifstream sym_file("Log.sym");
	std::string cur_proc_str;
	std::vector<std::string> proc_names;
	proc_names.push_back("Idle");
	while (sym_file >> cur_proc_str)
	{
		proc_names.push_back(cur_proc_str);
	}

	std::mt19937 Rand;
	std::uniform_int_distribution<> index_dist(0, static_cast<int>(proc_names.size() - 1));
	std::uniform_real_distribution<float> time_dist(0.01f, 100.0f);

	std::uniform_real_distribution<float> time_mult_dist(0.2f, 1.3f);
	std::vector<float> time_proc_mults(proc_names.size());
	for (size_t i = 0; i < time_proc_mults.size(); i++)
	{
		time_proc_mults[i] = time_mult_dist(Rand);
	}

	remove("Log.out");
	std::ofstream OutFile("Log.out");
	for (size_t i = 0; i < 10000; i++)
	{
		int r_index = index_dist(Rand);
		OutFile << proc_names[r_index] << std::endl << r_index - 1 << std::endl << '-' << std::endl << time_dist(Rand) * time_proc_mults[r_index] << std::endl << std::endl;
	}
}