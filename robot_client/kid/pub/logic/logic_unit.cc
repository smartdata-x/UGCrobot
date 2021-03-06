//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月15日 Author: kerry

#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sstream>
#include <string>
#include "logic/logic_unit.h"
#include "logic/logic_comm.h"
#include "basic/md5sum.h"
#include "basic/radom_in.h"
#include "net/comm_head.h"

namespace logic {

typedef struct server* (*CoreGetServer)();
struct threadrw_t* SendUtils::socket_lock_ = NULL;
struct server* CoreSoUtils::srv = NULL;

void CoreSoUtils::InitSRV() {
    basic::libhandle handle_lancher = NULL;
    handle_lancher = basic::load_native_library("./plugins/core/core.so");
    if (handle_lancher == NULL) {
        LOG_ERROR("Can't load path core.so\n");
        return;
    }
    CoreGetServer entry_point = (CoreGetServer) basic::get_function_pointer(
            handle_lancher, "get_server");
    if (entry_point == NULL) {
        LOG_ERROR("Can't get function get_server");
        return;
    }
    srv = entry_point();
    LOG_DEBUG2("InitSRV srv=%p", srv);
}

struct server* CoreSoUtils::GetSRV() {
    if (NULL == CoreSoUtils::srv) {
        InitSRV();
    }
    return CoreSoUtils::srv;
}

int32 SendUtils::SendFull(int socket, const char *buffer, size_t nbytes) {
    return CoreSoUtils::GetSRV()->send_msg(socket, buffer, nbytes);
}

bool SendUtils::SendBytes(int socket, const void* bytes, int32 len,
                          const char* file, int32 line) {
    if (socket <= 0 || bytes == NULL || len <= 0)
        return false;

    int32 ret = SendFull(socket, reinterpret_cast<const char*>(bytes), len);
    if (ret != len) {
        LOG_ERROR("Send bytes failed");
        return false;
    }
    return true;
}

bool SendUtils::SendMessage(int socket, struct PacketHead *packet,
                            const char* file, int32 line) {
    bool r;
    void *packet_stream = NULL;
    int32 packet_stream_length = 0;
    int ret = 0;
    bool r1 = false;
    if (socket <= 0 || packet == NULL)
        return false;

    if (!packet->PackStream(&packet_stream, packet_stream_length)) {
        LOG_ERROR2("Call PackStream failed in %s:%d", file, line);
        r = false;
        goto MEMFREE;
    }

    packet->packet_length_ = packet_stream_length;

//    LOG_DEBUG2("Send Msg Success, Opcode = %d, Length = %d", packet->operate_code_,
//            packet->packet_length_);

    ret = SendFull(socket, reinterpret_cast<char*>(packet_stream),
                   packet_stream_length);
    net::PacketProsess::HexEncode(packet_stream, packet_stream_length);
    if (0 != ret) {
        LOG_ERROR2("Sent msg failed in %s:%d", file, line);
        r = false;
        goto MEMFREE;
    } else {
        r = true;
        goto MEMFREE;
    }

    MEMFREE: if (packet_stream) {
        free(packet_stream);
        packet_stream = NULL;
    }
    return r;
}

void* SomeUtils::GetLibraryFunction(const std::string& library_name,
                                    const std::string& func_name) {
    void* engine = NULL;
    basic::libhandle handle_lancher = NULL;
    handle_lancher = basic::load_native_library(library_name.c_str());

    if (handle_lancher == NULL) {
        LOG_ERROR2("Cant't load path %s", library_name.c_str());
        return false;
    }

    engine = basic::get_function_pointer(handle_lancher, func_name.c_str());

    if (engine == NULL) {
        LOG_ERROR2("Can't find %s", func_name.c_str());
        return NULL;
    }
    return engine;
}

void SomeUtils::CreateToken(const int64 uid, const std::string& password,
                            std::string* token) {
    std::stringstream os;
    os << uid << base::SysRadom::GetInstance()->GetRandomID() << password;
    base::MD5Sum md5(os.str());
    (*token) = md5.GetHash();
}

bool SomeUtils::VerifyToken(const int64 uid, const std::string& password,
                            const std::string& token) {
    return true;
}

int SomeUtils::GetAddressBySocket(const int sock, std::string &ip) {

    struct sockaddr_storage sa;
    int salen = sizeof(sa);
    if (::getpeername(sock, (struct sockaddr*) &sa, (socklen_t *) &salen)
            == -1) {
        ip = "?";
        return -1;
    }

    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in*) &sa;
        ip = ::inet_ntoa(s->sin_addr);
        return 0;
    }
    return -1;
}

int SomeUtils::GetPortBySocket(const int socket, int16 &port) {
    struct sockaddr_storage sa;
    int salen = sizeof(sa);
    if (::getpeername(socket, (struct sockaddr*) &sa, (socklen_t *) &salen)
            == -1) {
        port = -1;
        return -1;
    }

    port = 0;
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in*) &sa;
        port = ::ntohs(s->sin_port);
        return 0;
    }
    return -1;
}

std::string SomeUtils::GetLocalTime(const time_t current_time) {
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y/%m/%d %H:%M:%S", localtime(&current_time));
    std::string time(tmp);
    return time;
}

long SomeUtils::GetCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

std::string SomeUtils::GetBackUpFileName(){
    time_t now = time(NULL);
    char tmp[64];
    strftime( tmp, sizeof(tmp), "%Y-%m-%d", localtime(&now));
    std::string time(tmp);
    return time + ".log";;
}

}  //  namespace logic
