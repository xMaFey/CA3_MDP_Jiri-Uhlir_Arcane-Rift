// ============================================
// Name: Jiri Uhlir
// Student ID: D00260335
// ============================================

#include "core_net_pch.hpp"
#include "string_utils.hpp"
#include "socket_address.hpp"
#include "socket_address_factory.hpp"
#include "socket_util.hpp"
#include "udp_socket.hpp"
#include "memory_bit_stream.hpp"
#include "timing.hpp"
#include "weighted_timed_moving_average.hpp"
#include "robo_math.hpp"

float kDesiredFrameTime = 0.0166f;
#if !_WIN32
#include <chrono>
using namespace std::chrono;
#endif

Timing	Timing::sInstance;

namespace
{
#if _WIN32
	LARGE_INTEGER sStartTime = { 0 };
#else
	high_resolution_clock::time_point sStartTime;
#endif
}

Timing::Timing()
{
#if _WIN32
	LARGE_INTEGER perfFreq;
	QueryPerformanceFrequency(&perfFreq);
	mPerfCountDuration = 1.0 / perfFreq.QuadPart;

	QueryPerformanceCounter(&sStartTime);

	mLastFrameStartTime = GetTime();
#else
	sStartTime = high_resolution_clock::now();
#endif
}

void Timing::Update()
{

	double currentTime = GetTime();

	mDeltaTime = (float)(currentTime - mLastFrameStartTime);

	//frame lock at 60fps
	while (mDeltaTime < kDesiredFrameTime)
	{
		currentTime = GetTime();

		mDeltaTime = (float)(currentTime - mLastFrameStartTime);
	}

	mLastFrameStartTime = currentTime;
	mFrameStartTimef = static_cast<float>(mLastFrameStartTime);

}

double Timing::GetTime() const
{
#if _WIN32
	LARGE_INTEGER curTime, timeSinceStart;
	QueryPerformanceCounter(&curTime);

	timeSinceStart.QuadPart = curTime.QuadPart - sStartTime.QuadPart;

	return timeSinceStart.QuadPart * mPerfCountDuration;
#else
	auto now = high_resolution_clock::now();
	auto ms = duration_cast<milliseconds>(now - sStartTime).count();
	//a little uncool to then convert into a double just to go back, but oh well.
	return static_cast<double>(ms) / 1000;
#endif
}