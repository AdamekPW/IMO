#pragma once

#include <chrono>

class Timer
{
private:
	bool _hasStarted = false;
	bool _hasEnded = false;
	std::chrono::high_resolution_clock::time_point _start;
	std::chrono::high_resolution_clock::time_point _end;

public:
	void Start();
	void End();
	std::chrono::microseconds GetResult();
};

