#include "Timer.h"


void Timer::Start()
{
	this->_hasStarted = true;
	this->_start = std::chrono::high_resolution_clock::now();
}

void Timer::End()
{
	this->_hasEnded = true;
	this->_end = std::chrono::high_resolution_clock::now();
}

std::chrono::microseconds Timer::GetResult()
{
	if (this->_hasStarted && !this->_hasEnded)
	{
		this->End();
	}

	if (this->_hasStarted && this->_hasEnded)
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(this->_end - this->_start);
	}

	std::chrono::microseconds czas(0);
	return czas;
}