#include <iostream>
#include <string>
#include <vector>

struct RequestURL {
    enum {
        RELATIVE = 0,
        HTTP  = 80,
        HTTPs = 443
    } scheme = RELATIVE;
    std::string host;
    unsigned    port;
    std::string path;
    std::string query_string;

    static RequestURL fromString(const std::string& str);
};

struct HttpRequest {
    enum {
        UNDEFINED,
        GET,
        POST,
        HEAD,
        PUT,
        DELETE,
        OPTIONS,
        PATCH
    } method = UNDEFINED;

    RequestURL url;
    float version = 0;
    
    std::string host;
    std::string user_agent;
    std::string refer;
    std::string accept;
    std::string accept_language;
    std::string accept_encoding;

    enum {
        KEEP_ALIVE,
        CLOSE
    } connection = KEEP_ALIVE;
    size_t upgrade_insecure_requests = 0;

    size_t content_length = BODY_LENGTH;
    std::string content_category;
    std::string content_type;
    std::string content_options;
    std::string content_encoding;

    std::string other;

    std::vector<char> data;

    static const size_t BODY_LENGTH = SIZE_MAX;
    static HttpRequest readRequest(std::basic_istream<char>& in);
};

struct HttpResponse {
    enum {
        PROCESSING     = 102,
        OK             = 200,
        CREATED        = 201,
        ACCEPTED       = 202,
        NO_CONTENT     = 204,
        BAD_REQUEST    = 400,
        FORBIDDEN      = 403,
        NOT_FOUND      = 404,
        METHOD_NOT_ALLOWED = 405,
        CONFLICT       = 409,
        INTERNAL_ERROR = 500
    } status = OK;

    float version = 0;

    enum {
        KEEP_ALIVE,
        CLOSE
    } connection = KEEP_ALIVE;

    size_t content_length = 0;
    std::string content_category;
    std::string content_type;
    std::string content_options;
    std::string content_encoding;
    std::string content_disposition;

    std::string other;
    std::vector<char> data;

    static std::string getResponseHeader(const HttpResponse& response);
    static void setTypeFromExtension(HttpResponse& response, const std::string& extension);
};