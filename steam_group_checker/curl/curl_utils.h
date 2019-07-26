#pragma once
#include <string>
#include <stdio.h>

#define CURL_STATICLIB
#include "curl.h"

#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Ws2_32.lib")

struct curl_ret
{
	std::string body = "";
	long response_code = 0;
};

class curl_utils
{
public:
	curl_utils() { curl = curl_easy_init(); }
	~curl_utils() { if (curl) curl_easy_cleanup(curl); }

	curl_ret request(const std::string& url, const std::string& data = "")
	{
		curl_ret ret;
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			if (data != "")
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret.body);

			res = curl_easy_perform(curl);

			if (res != CURLE_OK) 
			{
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
				return curl_ret{ "invalid", -1 };
			} else {
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ret.response_code);
			}
		}

		return ret;
	}

private:
	static size_t write_callback(void* contents, size_t size, size_t nmemb, void *userp)
	{
		reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<char*>(contents), size * nmemb);
		return size * nmemb;
	}

	CURL* curl;
	CURLcode res;
};