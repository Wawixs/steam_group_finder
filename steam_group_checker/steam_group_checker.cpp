#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "utils/utils.h"
#include "curl/curl_utils.h"
#include "rapidxml/rapidxml.hpp"
using namespace rapidxml;

std::unique_ptr<blocking_queue<std::string>> queue = std::make_unique<blocking_queue<std::string>>(1);

void async_logging()
{
	while (true)
	{
		std::string str = queue->pop(); // Pop from queue and store inside str to use it under
		if (str.empty())
			continue;

		// Write to file
		std::ofstream groups_found("groups_found.txt", std::ofstream::app);
		groups_found << str.c_str();
		groups_found.close();

		// Print to console :-)
		printf(str.c_str());

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

int group_id = 0;
int max_threads = 15;
int threads = 0;
bool get_owner_info = false;
void thread_routine()
{
	std::unique_ptr<curl_utils> curl = std::make_unique<curl_utils>();
	while (!curl)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	// Variables definition
	std::string url = "https://steamcommunity.com/gid/" + std::to_string(group_id);
	int saved_group_id = group_id;
	group_id += 1;

	// Send a GET requests to the URLs
	curl_ret ret_xml = curl->request(url + "/memberslistxml/?xml=1");
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	curl_ret ret = curl->request(url);

	if (ret.body.find("You don't have permission to access") != std::string::npos || ret.response_code == 429 || ret_xml.response_code == 429)
	{
		printf_s("[Error] Failed request, steam servers blocked your IP for some time (Group ID: %i | Response code: %i - %i)\n", saved_group_id, ret.response_code, ret_xml.response_code);
		--group_id;
		--threads;
		return;
	}

	if (ret.response_code != 200 || ret_xml.response_code != 200)
	{
		printf_s("[Error] Failed CURL request (Group ID: %i)\n", saved_group_id);
		--threads;
		return;
	}

	// Convert the string to char* array
	char* copied_char_str = new char[ret_xml.body.size() + 1];
	strcpy_s(copied_char_str, ret_xml.body.size() + 1, ret_xml.body.c_str());

	std::string group_infos = "\n________________\n";
	try {
		// Parse XML string
		xml_document<> doc;
		doc.parse<parse_default>(copied_char_str);
		xml_node<>* group_details = doc.first_node()->first_node("groupDetails");

		// Access nodes
		{
			std::string group_name = group_details->first_node("groupName")->first_node()->value();
			std::string group_url = std::string("https://steamcommunity.com/groups/") + group_details->first_node("groupURL")->first_node()->value();
			std::string member_count = group_details->first_node("memberCount")->first_node()->value();
			std::string members_online = group_details->first_node("membersOnline")->first_node()->value();

			std::string spl1t = split(ret.body.c_str(), "<span class=\"grouppage_header_abbrev\" >").at(1);
			std::string clan_tag = split(spl1t.c_str(), "</span>").at(0);

			// Check if we want owner info
			if (get_owner_info)
			{
				// Get first id64 from XML (obviously owner is always first)
				std::string owner_id64 = doc.first_node()->first_node("members")->first_node()->value();
				
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				ret = curl->request("https://steamcommunity.com/profiles/" + owner_id64); // GET request to URL

				// Get name and level by splitting HTML page
				std::string name;
				std::string level;

				spl1t = split(ret.body.c_str(), "<span class=\"actual_persona_name\">").at(1);
				name = split(spl1t.c_str(), "</span>").at(0);
				try {
					spl1t = split(ret.body.c_str(), "<span class=\"friendPlayerLevelNum\">").at(1);
					level = split(spl1t.c_str(), "</span>").at(0);
				} catch (...) {
					level = "Private";
				}

				// Append owner info to group_infos string
				group_infos.append(std::string("< Owner infos >\n") \
					+ "Name: " + name + "\n" \
					+ "Level: " + level + "\n" \
				);
			}

			// Put all strings inside one for priting
			group_infos.append(std::string("< Group infos >\n") \
				+ "Name: " + group_name + "\n" \
				+ "Clan tag: " + clan_tag + "\n" \
				+ "URL: " + group_url + "\n" \
				+ "Member count: " + member_count + "\n" \
				+ "Online members: " + members_online + "\n" \
				+ "Group ID: " + std::to_string(saved_group_id) + "\n" \
				+ "________________\n");

			queue->push(group_infos); // Push to blocking queue
		}
	} catch (std::exception ex) {
		printf_s("[Error] Invalid group (ID: %i) (%s)\n", saved_group_id, ex.what());
	}

	// Avoid memory leak of course
	if (copied_char_str)
		delete[] copied_char_str;

	--threads;
}

void thread_manager()
{
	std::thread log_thread(async_logging);
	log_thread.detach();

	while (true)
	{
		std::string title = "Steam group finder (" + std::to_string(threads) + "threads running)";
		SetConsoleTitleA(title.c_str());

		if (threads < max_threads)
		{
			std::thread new_thread(thread_routine);
			new_thread.detach();
			threads += 1;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

int main()
{
	std::cout << R"(
           |`-.._____..-'|
           :  > .  ,  <  :
           `./___`' ___\,'
            |(___)-(___)|
            ; __  .  __ :
            `. .' - `. ,'
              `,_____.'
              / \___/ \
             /    $    :
            :          |_
           ,|  .    .  | \
          : :   \   |  |  :
          |  \   :`-;  ;  |
          :   :  | /  /   ;
           :-.'  ;'  / _,'`------.
           `'`''-`'''-'-''--.---  )
           by wawixs        `----'
)" << '\n';

	curl_global_init(CURL_GLOBAL_DEFAULT);

	try {
		std::cout << "Starting group ID (ex 999): \n";
		std::cin >> group_id;

		std::cout << "Get group owner info [0/1]: \n";
		std::cin >> get_owner_info;

		std::cout << "Running more than 1 thread will disorder the groups (it might check id 10 before id 8 etc)\n";
		std::cout << "Maximum threads [number]: \n";
		std::cin >> max_threads;

		thread_manager();
	} catch (std::exception ex) {
		printf_s("[Error] %s\n", ex.what());
		system("pause");
	}

	curl_global_cleanup();
}