#pragma once
#include <iostream>
#include <string>

using namespace std;

namespace utils
{
	// Utils
	vector<string> split(string haystack, string needle)
	{
		vector<string> res;

		int start, end = -1 * needle.size();
		do
		{
			start = end + needle.size();
			end = haystack.find(needle, start);
			res.push_back(haystack.substr(start, end - start));
		} while (end != -1);

		return res;
	};
}