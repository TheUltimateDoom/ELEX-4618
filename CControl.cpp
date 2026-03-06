#include "stdafx.h"
#include "CControl.h"
#include <chrono>


// Constructors
CControl::CControl()
{
}

CControl::~CControl()
{
}



void CControl::init_com(int comport)
{
    std::string portName = "COM" + std::to_string(comport);
    if (_com.open(portName, 115200)) 
    {
        std::cout << "Connected to " << portName << " at 115200 baud rate." << std::endl;
    }
}

bool CControl::get_data(int type, int channel, int& result)
{
	std::string command = "G " + std::to_string(type) + " " + std::to_string(channel) + "\n";
	_com.flush();
	_com.write(command.c_str(), command.length());

	char buffer[100];
	int totalBytes = 0;

	// Start the timeout clock (giving it a generous 10ms max)
	auto start_wait = std::chrono::steady_clock::now();

	// Keep looping until the buffer is full OR we break out
	while (totalBytes < 99)
	{
		// Read whatever is currently waiting in the serial port
		int bytesRead = _com.read(buffer + totalBytes, 99 - totalBytes);

		if (bytesRead > 0)
		{
			totalBytes += bytesRead;
			buffer[totalBytes] = '\0'; // Cap it off as a valid string

			// Check if the microcontroller sent the newline character yet
			if (strchr(buffer, '\n') != NULL)
			{
				break; // The complete message has arrived!
			}
		}

		// Timeout check: If 10ms pass and we still don't have a full message, bail out
		if (std::chrono::steady_clock::now() - start_wait > std::chrono::milliseconds(10))
		{
			break;
		}
	}

	// Now we parse the completed string
	if (totalBytes > 0)
	{
		char* startPtr = strchr(buffer, 'A');

		if (startPtr != NULL)
		{
			int resType, resChannel, resValue;
			if (sscanf_s(startPtr, "A %d %d %d", &resType, &resChannel, &resValue) == 3)
			{
				result = resValue;
				return true;
			}
		}
	}
	return false;
}

bool CControl::set_data(int type, int channel, int val)
{
	std::string command = "S " + std::to_string(type) + " " + std::to_string(channel) + " " + std::to_string(val) + "\n";
	return (_com.write(command.c_str(), command.length()) > 0);
}

float CControl::get_analog(int channel)
{
	int result = 0;
	// Call get_data to get the raw integer value (0-4095)
	if (get_data(ANALOG, channel, result))
	{
		// Return the percentage of the 12-bit ADC full scale
		return (static_cast<float>(result) / 4095.0f) * 100.0f;
	}
	return 0.0f; // Return 0 if communication fails
}


// 0: everything worked
// 1: button not pressed
// 2: serial comm failed
int CControl::get_button(int channel)
{
	int currentState = 1;
	static DWORD lastPressTime = 0;
	static int lastState = 1;

	// 1. Get the current physical state of the button
	if (get_data(DIGITAL, channel, currentState))
	{
		DWORD currentTime = GetTickCount();
		int result = BUTTON_PRESS_NONE; // Default result

		// Check for a "Falling Edge" (Transition from 1 to 0)
		if (currentState == 0 && lastState == 1)
		{
			// Only count it if 1000ms has passed
			if (currentTime - lastPressTime > 1000)
			{
				lastPressTime = currentTime;
				result = BUTTON_PRESS_SUCCESS;
			}
		}

		lastState = currentState; // Always update lastState if comms worked
		return result;
	}

	return BUTTON_PRESS_FAIL; // get_data failed
}

bool CControl::auto_init()
{
	std::cout << "Scanning for microcontroller..." << std::endl;

	for (int i = 1; i < 16; i++)
	{
		std::string portName = "COM" + std::to_string(i);

		if (_com.open(portName, 115200))
		{
			_com.flush();

			// 2. Try probing 3 times before deciding this port is "wrong"
			for (int retry = 0; retry < 3; retry++)
			{
				int result = 0;
				if (get_data(DIGITAL, S1, result)) // Try reading S1
				{
					std::cout << "\nSuccess! Board found on " << portName << std::endl;
					return true;
				}
				Sleep(100);
			}
		}
	}
	return false;
}