#include "HttpClient.h"

#include <windows.h>
#include <wininet.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace FlipOut {

    std::string HttpClient::Get(const std::string& url) {
        HINTERNET hInternet = InternetOpenA("FlipOut/1.0",
            INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) return "";

        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                      INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;

        HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, flags, 0);
        if (!hUrl) {
            InternetCloseHandle(hInternet);
            return "";
        }

        std::string result;
        char buffer[8192];
        DWORD bytesRead;
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            result.append(buffer, bytesRead);
        }

        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return result;
    }

    HttpResponse HttpClient::GetEx(const std::string& url) {
        HttpResponse result;
        HINTERNET hInternet = InternetOpenA("FlipOut/1.0",
            INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) return result;

        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                      INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;

        HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, flags, 0);
        if (!hUrl) {
            InternetCloseHandle(hInternet);
            return result;
        }

        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &statusCode, &statusSize, NULL);
        result.status_code = (int)statusCode;

        char buffer[8192];
        DWORD bytesRead;
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            result.body.append(buffer, bytesRead);
        }

        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return result;
    }

    std::string HttpClient::AuthenticatedGet(const std::string& url, const std::string& api_key) {
        if (api_key.empty()) return "";
        std::string sep = (url.find('?') != std::string::npos) ? "&" : "?";
        return Get(url + sep + "access_token=" + api_key);
    }

    std::string HttpClient::CheckApiResponse(const HttpResponse& resp) {
        if (resp.status_code == 0)
            return "No response (network error or API offline)";
        if (resp.status_code == 502 || resp.status_code == 503)
            return "GW2 API is temporarily unavailable (maintenance)";
        if (resp.status_code >= 500)
            return "GW2 API server error (HTTP " + std::to_string(resp.status_code) + ")";
        if (resp.body.empty())
            return "Empty response from API";
        if (resp.body.size() > 0 && resp.body[0] == '<')
            return "GW2 API returned unexpected response (possible maintenance)";
        try {
            json j = json::parse(resp.body);
            if (j.is_object() && j.contains("text"))
                return "GW2 API: " + j["text"].get<std::string>();
        } catch (...) {
            return "GW2 API returned invalid response";
        }
        return "";
    }

}
