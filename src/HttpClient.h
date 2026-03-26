#pragma once

#include <string>

namespace FlipOut {

    struct HttpResponse {
        int status_code = 0;
        std::string body;
    };

    class HttpClient {
    public:
        // Simple GET returning body only (empty on failure)
        static std::string Get(const std::string& url);

        // Extended GET returning status code + body
        static HttpResponse GetEx(const std::string& url);

        // GET with GW2 API key appended as query param
        static std::string AuthenticatedGet(const std::string& url, const std::string& api_key);

        // Check if an API response indicates an error; returns empty string if OK, else error message
        static std::string CheckApiResponse(const HttpResponse& resp);
    };

}
