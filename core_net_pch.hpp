// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <list>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <iostream>

using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::vector;
using std::queue;
using std::list;
using std::deque;
using std::unordered_map;
using std::unordered_set;

typedef int socklen_t;

#ifndef NO_ERROR
#define NO_ERROR 0
#endif

#define LOG(...) StringUtils::Log(__VA_ARGS__)