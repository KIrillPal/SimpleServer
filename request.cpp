#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <exception>
#include <cstdlib>
#include <cerrno>
#include <algorithm>
#include "request.hpp"

std::string lowercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
           [](unsigned char c) { return std::tolower(c); });
    return std::move(str);
}

void alertNote(const std::string& message) {
    std::cout << "Note: " << message << '\n';
}

void alertInvalidHeader(const std::string& header, const std::string& value) {
    alertNote("got invalid field '" + header + "' with value '" + value + "'");
}

std::string getStatusMessage(const HttpResponse& response) {
    if (response.status == HttpResponse::OK) {
        return "OK";
    }
    if (response.status == HttpResponse::NOT_FOUND) {
        return "Not Found";
    }
    if (response.status == HttpResponse::FORBIDDEN) {
        return "Forbidden";
    }
    if (response.status == HttpResponse::INTERNAL_ERROR) {
        return "Internal Error";
    }
    return "";
}

bool setMethod(HttpRequest& request, const std::string& method_name) {
    if (method_name == "GET")
        request.method = HttpRequest::GET;
    else if (method_name == "POST")
        request.method = HttpRequest::POST;
    else if (method_name == "HEAD")
        request.method = HttpRequest::HEAD;
    else if (method_name == "PUT")
        request.method = HttpRequest::PUT;
    else if (method_name == "DELETE")
        request.method = HttpRequest::DELETE;
    else if (method_name == "OPTIONS")
        request.method = HttpRequest::OPTIONS;
    else if (method_name == "PATCH")
        request.method = HttpRequest::PATCH;
    else {
        request.method = HttpRequest::UNDEFINED;
        return true;
    }
    return false;
}

bool setHttpVersion(HttpRequest& request, const std::string& version_string) {
    const float MIN_VERSION = 1.f;
    const float MAX_VERSION = 10.f;

    if (version_string.substr(0, 5) != "HTTP/") {
        return true;
    }
    std::string version = version_string.substr(5);
    request.version = strtof(version.c_str(), NULL);
    if (errno) {
        return true;
    }
    return (request.version < MIN_VERSION || MAX_VERSION < request.version);
}

bool setProtocol(RequestURL& url, const std::string& protocol) {
    if (protocol == "http")
        url.scheme = RequestURL::HTTP;
    else if (protocol == "https")
        url.scheme = RequestURL::HTTPs;
    else return true;
    return false;
}

bool setHostAndPort(RequestURL& url, const std::string& address) {
    const unsigned long MIN_PORT = 1;
    const unsigned long MAX_PORT = 65535;

    long port_pos = address.find(":");
    if (port_pos != std::string::npos) {
        unsigned long port = strtol(address.substr(port_pos + 1).c_str(), NULL, 10);
        if (errno || port < MIN_PORT || MAX_PORT < port) {
            return true;
        }
        url.port = port;
    }
    url.host = address.substr(0, port_pos);
    return false;
}

void readRequestBody(std::basic_istream<char>& in, HttpRequest& request) {
    try {
        request.data.resize(request.content_length);
        in.read(request.data.data(), request.content_length);
        size_t got = in.gcount();

        if (got < request.content_length) {
            std::string message = "could read only " + std::to_string(got);
            message += " bytes out of Content-Length=" + std::to_string(request.content_length);
            alertNote(message);
        }
    } catch (std::exception& ex) {
        std::string message = "Failed to load " + std::to_string(request.content_length);
        message += std::string(" bytes of package body: ") + ex.what();
        throw std::runtime_error(message);
    }
}

RequestURL RequestURL::fromString(const std::string &str) {
    RequestURL url;

    long root_start = 0;
    long protocol_size = str.find("://");
    if (protocol_size != std::string::npos) {
        std::string protocol = lowercase(str.substr(0, protocol_size));
        if (setProtocol(url, protocol)) {
            throw std::runtime_error("invalid protocol " + protocol + ". Expected 'http' or etc.");
        }

        auto offset = protocol_size + 3;

        root_start = str.find('/', offset);
        if (root_start == std::string::npos) {
            root_start = str.size();
        }
        std::string address = lowercase(str.substr(offset, root_start - offset));
        if (setHostAndPort(url, address)) {
            throw std::runtime_error("invalid port of address " + address);
        }
    }

    long query_pos = str.find("?");
    url.path = str.substr(root_start, query_pos - root_start);
    if (query_pos != std::string::npos) {
        url.query_string = str.substr(query_pos + 1);
    }
    return std::move(url);
}

void setContentType(HttpRequest& request, std::string& value) {
    std::string type;
    std::stringstream value_stream(value);
    value_stream >> type;
    if (type.size() < 2) {
        alertInvalidHeader("Content-Type", value);
    }
    if (type.back() == ';')
        type.pop_back();
    long delim_pos = type.find('/');
    if (delim_pos != std::string::npos) {
        request.content_category = type.substr(0, delim_pos);
        request.content_type     = type.substr(delim_pos);
    }

    if (request.content_category.empty())
        request.content_category = "*";
    else if (request.content_type.empty())
        request.content_type = "*";

    std::string opt;
    while (value_stream >> opt) {
        request.content_options += opt;
    }
}

void setConnection(HttpRequest& request, std::string& value) {
    value = lowercase(value);
    std::string answer;
    std::stringstream(value) >> answer;
    if (answer == "keep-alive")
        request.connection = HttpRequest::KEEP_ALIVE;
    else if (answer == "close")
        request.connection = HttpRequest::CLOSE;
    else
        request.connection = HttpRequest::KEEP_ALIVE;
}

void setContentLength(HttpRequest& request, std::string& value) {
    std::string val_str;
    std::stringstream(value) >> val_str;
    if (val_str == "length") {
        request.content_length = HttpRequest::BODY_LENGTH;
    }
    else if (!(std::stringstream(value) >> request.content_length)) {
        alertInvalidHeader("Content-Length", value);
    }
}

bool setArgument(HttpRequest& request, const std::string& line) {
    long delim_pos = line.find(": ");
    if (delim_pos == std::string::npos) {
        return true;
    }
    std::stringstream argument_stream(line.substr(0, delim_pos));
    std::string argument, value;
    argument_stream >> argument;
    argument = lowercase(argument);
    value = line.substr(delim_pos + 2);

    //request headers
    if (argument == "host") {
        request.host = value;
    }
    else if (argument == "user-agent") {
        request.user_agent = value;
    }
    else if (argument == "refer") {
        request.refer = value;
    }
    else if (argument == "accept") {
        request.accept = value;
    }
    else if (argument == "accept-language") {
        request.accept_language = value;
    }
    else if (argument == "accept-encoding") {
        request.accept_encoding = value;
    }
    // general headers
    else if (argument == "connection") {
        setConnection(request, value);
    }
    else if (argument == "upgrade-insecure-requests") {
        if (!(std::stringstream(value) >> request.upgrade_insecure_requests))
            alertInvalidHeader("Upgrade-Insecure-Requests", value);
    }
    // representation headers
    else if (argument == "content-length") {
        setContentLength(request, value);
    }
    else if (argument == "content-type") {
        setContentType(request, value);
    }
    else if (argument == "content-encoding") {
        request.content_encoding = value;
    }
    else {
        request.other += line + '\n';
    }
    return false;
}


HttpRequest HttpRequest::readRequest(std::basic_istream<char>& in) {
    std::string method_name, url_string, http_version;

    HttpRequest request;
    in >> method_name >> url_string >> http_version;
    if (setMethod(request, method_name)) {
        throw std::runtime_error("invalid method '" + method_name + "'. Expected 'GET' or etc.");
    }

    if (setHttpVersion(request, http_version)) {
        throw std::runtime_error("Invalid HTTP version. As example 'HTTP/2'");
    }
    request.url = RequestURL::fromString(url_string);
    int line_number = 2;
    std::string line;
    std::getline(in, line, '\r');

    while (std::getline(in, line, '\r') && (!line.empty() && line == "\n")) {
        if (setArgument(request, line)) {
            std::string message = "invalid format of argument line ";
            message += std::to_string(line_number) + " '" + line + "'";
            throw std::runtime_error(message);
        }
        line.clear();
        ++line_number;
    }

    if (request.content_length > 0) {
        if (request.content_length == HttpRequest::BODY_LENGTH) {
            std::string data(std::istreambuf_iterator<char>(in), {});
            request.data = std::move(std::vector<char>(data.begin(), data.end()));
            request.content_length = request.data.size();
        }
        else readRequestBody(in, request);

    }
    return std::move(request);
}

std::string HttpResponse::getResponseHeader(const HttpResponse &response) {
    static const char newline[] = "\r\n";

    std::stringstream out;
    out << "HTTP/" << response.version << " ";
    out << (unsigned)response.status << " " << getStatusMessage(response) << newline;

    if (response.connection == HttpResponse::KEEP_ALIVE)
         out << "Connection: Keep-Alive" << newline;
    else out << "Connection: Close" << newline;

    if (response.content_length != 0) {
        out << "Content-Length: " << response.content_length << newline;
        out << "Content-Type: " << response.content_category << "/" << response.content_type;
        if (!response.content_options.empty()) {
            out << "; " << response.content_options;
        }
        out << newline;
    }
    if (!response.content_encoding.empty()) {
        out << "Content-Encoding: " << response.content_encoding.c_str() << newline;
    }
    out << response.other.c_str();
    out << newline;
    return std::move(out.str());
}

void HttpResponse::setTypeFromExtension(HttpResponse& response, const std::string& extension) {
    static const std::vector<std::string> applicationTypes =
            {
                    "java-archive",
                    "EDI-X12",
                    "javascript",
                    "octet-stream",
                    "ogg",
                    "pdf",
                    "zip",
            };
    static const std::vector<std::string> audioTypes =
            {
                    "mpeg",
                    "mp3",
                    "wav",
                    "x-ms-wma",
                    "x-wav"
            };
    static const std::vector<std::string> imageTypes =
            {
                    "jpeg",
                    "gif",
                    "png",
                    "tiff"
            };
    static const std::vector<std::string> videoTypes =
            {
                "mp4",
                "mpeg",
                "mkv",
                "avi",
                "webm"
            };
    static const std::vector<std::string> multipartTypes =
            {
                    "form-data",
                    "mixed",
                    "alternative",
                    "related",
            };

    response.content_type = extension;

    if (std::find(applicationTypes.begin(), applicationTypes.end(), extension) != applicationTypes.end()) {
        response.content_category = "application";
        return;
    }
    if (std::find(audioTypes.begin(), audioTypes.end(), extension) != audioTypes.end()) {
        response.content_category = "audio";
        return;
    }
    if (std::find(videoTypes.begin(), videoTypes.end(), extension) != videoTypes.end()) {
        response.content_category = "video";
        return;
    }
    if (std::find(multipartTypes.begin(), multipartTypes.end(), extension) != multipartTypes.end()) {
        response.content_category = "multipart";
        response.content_options = "boundary=something";
        return;
    }
    response.content_category = "text";
    response.content_options = "charset=utf-8";
}
