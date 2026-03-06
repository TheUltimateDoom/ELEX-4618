#include "stdafx.h"
#include "CPong.h"

#include "cvui.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <Windows.h>


CPong::CPong(cv::Size size, int comPort)
{
	// Initialize serial communication
	_control.init_com(comPort);

	// Create the canvas
	_canvas = cv::Mat::zeros(size, CV_8UC3);

	// Initialize Game State
	_score_player = 0;
	_score_computer = 0;
	_game_over = false;
	_reset_pending = false;

	// Initialize Timing
	_last_tick = cv::getTickCount();
	_fps = 0.0;
	_thread_exit = false;

	// Initialize Hardware Data
	_joyY = 50.0f;
	_button_reset = 1;
	_button_reset_previous = 1;

	// Initialize ball properties
	_ball_radius = 20;
	_ball_velocity_magnitude = 250;
	_ball_position = cv::Point2f(size.width / 2.0f, size.height / 2.0f);
	_ball_velocity = cv::Point2f(_ball_velocity_magnitude * 0.707f, _ball_velocity_magnitude * 0.707f);

	// Initialize paddles
	int paddleW = 20;
	int paddleH = 100;
	_player_paddle = cv::Rect(size.width - paddleW - 20, size.height / 2 - paddleH / 2, paddleW, paddleH);
	_computer_paddle = cv::Rect(20, size.height / 2 - paddleH / 2, paddleW, paddleH);
}


void CPong::gpio()
{
	float temp_joyY = _control.get_analog(JOY_Y);
	int temp_btnResetRaw;
	_control.get_data(DIGITAL, S1, temp_btnResetRaw);

	_data_mutex.lock();
	_joyY = temp_joyY;
	_button_reset = temp_btnResetRaw;
	_data_mutex.unlock();
}


void CPong::update()
{
	// Set target frame time for 30 FPS
	auto end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(33);

	if (_reset_pending == true)
	{
		//Randomize direction
		float dirX = (rand() % 2 == 0) ? 1.0f : -1.0f;
		float dirY = (rand() % 2 == 0) ? 1.0f : -1.0f;

		_data_mutex.lock();

		// Put ball back in the middle
		_ball_position = cv::Point2f(_canvas.cols / 2.0f, _canvas.rows / 2.0f);

		// 45 degrees in a random quadrant
		_ball_velocity.x = dirX * (_ball_velocity_magnitude * 0.707f);
		_ball_velocity.y = dirY * (_ball_velocity_magnitude * 0.707f);

		_reset_pending = false;
		_data_mutex.unlock();
	}

	// Detect falling edge of the reset button to trigger a game reset
	if (_button_reset == 0 && _button_reset_previous == 1) 
	{
		_data_mutex.lock();
		_reset_pending = true;
		_score_player = 0;
		_score_computer = 0;
		_game_over = false;
		_ball_position = cv::Point2f(_canvas.cols / 2.0f, _canvas.rows / 2.0f);
		_data_mutex.unlock();
	}


	// Save previous button state for edge detection in the next frame
	_data_mutex.lock();
	_button_reset_previous = _button_reset;
	_data_mutex.unlock();


	// Check for game over condition (first to 5 points)
	if (_score_player >= 5 || _score_computer >= 5)
	{
		_data_mutex.lock();
		_game_over = true;
		_data_mutex.unlock();
	}

	// Main update logic 
	if (_game_over == false && _reset_pending == false)
	{

		_data_mutex.lock();

		// time since last frame in seconds
		double currentTick = cv::getTickCount();
		double dt = (currentTick - _last_tick) / cv::getTickFrequency();
		_last_tick = currentTick;

		if (dt > 0.001)
		{
			_fps = 1.0 / dt;
		}

		float currentSpeed = std::sqrt((_ball_velocity.x * _ball_velocity.x) + (_ball_velocity.y * _ball_velocity.y));

		if (currentSpeed > 0)
		{
			// Normalize velocity and apply magnitude to ensure consistent speed
			_ball_velocity.x = (_ball_velocity.x / currentSpeed) * _ball_velocity_magnitude;
			_ball_velocity.y = (_ball_velocity.y / currentSpeed) * _ball_velocity_magnitude;
		}

		// Move the Ball 
		_ball_position.x += _ball_velocity.x * dt;
		_ball_position.y += _ball_velocity.y * dt;

		// Top Wall Bounce
		if (_ball_position.y - _ball_radius < 0)
		{
			_ball_position.y = _ball_radius;         // Snap to edge to prevent getting stuck
			_ball_velocity.y = std::abs(_ball_velocity.y); // Force velocity downwards
		}
		// Bottom Wall Bounce
		else if (_ball_position.y + _ball_radius > _canvas.rows)
		{
			_ball_position.y = _canvas.rows - _ball_radius; // Snap to edge
			_ball_velocity.y = -std::abs(_ball_velocity.y); // Force velocity upwards
		}

		// --- SCORING (Left and Right Walls) ---
		// Right Wall (Computer Scores)
		if (_ball_position.x + _ball_radius > _canvas.cols)
		{
			_score_computer++;
			_reset_pending = true; // Flag for a reset
		}
		// Left Wall (Player Scores)
		else if (_ball_position.x - _ball_radius < 0)
		{
			_score_player++;
			_reset_pending = true; // Flag for a reset
		}


		// --- PADDLE COLLISIONS ---

		// Player Paddle Collision (Right side)
		// Check if the ball's bounding box overlaps the paddle's rectangle
		if (_ball_position.x + _ball_radius >= _player_paddle.x &&
			_ball_position.x - _ball_radius <= _player_paddle.x + _player_paddle.width &&
			_ball_position.y + _ball_radius >= _player_paddle.y &&
			_ball_position.y - _ball_radius <= _player_paddle.y + _player_paddle.height)
		{
			// Snap ball to the edge of the paddle to prevent it from getting stuck inside
			_ball_position.x = _player_paddle.x - _ball_radius;

			// Calculate exactly where the ball hit the paddle (relative to the paddle's center)
			float paddleCenterY = _player_paddle.y + (_player_paddle.height / 2.0f);
			float relativeIntersectY = paddleCenterY - _ball_position.y;

			// Normalize this value from -1.0 (bottom edge) to 1.0 (top edge)
			float normalizedIntersectY = relativeIntersectY / (_player_paddle.height / 2.0f);

			// Calculate the bounce angle (Max bounce angle is 45 degrees, which is ~0.785 radians)
			float bounceAngle = normalizedIntersectY * 0.785f;

			// Calculate new velocity based on the bounce angle while maintaining the ball's speed
			// X is negative because it's bouncing left, away from the player
			_ball_velocity.x = -_ball_velocity_magnitude * cos(bounceAngle);

			// Y is negative sine because OpenCV's Y-axis is inverted (0 is at the top)
			_ball_velocity.y = _ball_velocity_magnitude * -sin(bounceAngle);
		}

		// Computer Paddle Collision (Left side)
		if (_ball_position.x - _ball_radius <= _computer_paddle.x + _computer_paddle.width &&
			_ball_position.x + _ball_radius >= _computer_paddle.x &&
			_ball_position.y + _ball_radius >= _computer_paddle.y &&
			_ball_position.y - _ball_radius <= _computer_paddle.y + _computer_paddle.height)
		{
			// Snap ball to the edge of the paddle
			_ball_position.x = _computer_paddle.x + _computer_paddle.width + _ball_radius;

			float paddleCenterY = _computer_paddle.y + (_computer_paddle.height / 2.0f);
			float relativeIntersectY = paddleCenterY - _ball_position.y;

			float normalizedIntersectY = relativeIntersectY / (_computer_paddle.height / 2.0f);
			float bounceAngle = normalizedIntersectY * 0.785f;

			// X is positive because it's bouncing right, away from the computer
			_ball_velocity.x = _ball_velocity_magnitude * cos(bounceAngle);
			_ball_velocity.y = _ball_velocity_magnitude * -sin(bounceAngle);
		}

		// --- PADDLE MOVEMENT ---

		// Player paddle
		// Directly follows the joystick (with Y inverted because 0% is top and 100% is bottom)
		_player_paddle.y = ((100.0f - _joyY) / 100.0f) * (_canvas.rows - _player_paddle.height);

		// Computer Paddle
		// Calculate where we want to be
		float targetY = _ball_position.y - (_computer_paddle.height / 2);
		float computerSpeed = 6.0f;

		// Get the distance to the target
		float distance = targetY - _computer_paddle.y;

		// If the distance is smaller than our speed, snap to it (stops jitter)
		if (std::abs(distance) < computerSpeed)
		{
			_computer_paddle.y = targetY;
		}
		else
		{
			// Otherwise, move normally
			if (_computer_paddle.y < targetY)
			{
				_computer_paddle.y += computerSpeed;
			}
			else if (_computer_paddle.y > targetY)
			{
				_computer_paddle.y -= computerSpeed;
			}
		}

		// Clamp paddles to screen
		if (_player_paddle.y < 0)
		{
			_player_paddle.y = 0;
		}

		if (_player_paddle.y > _canvas.rows - _player_paddle.height)
		{
			_player_paddle.y = _canvas.rows - _player_paddle.height;
		}

		if (_computer_paddle.y < 0)
		{
			_computer_paddle.y = 0;
		}

		if (_computer_paddle.y > _canvas.rows - _computer_paddle.height)
		{
			_computer_paddle.y = _canvas.rows - _computer_paddle.height;
		}
		_data_mutex.unlock();
	}

	// Sleep normally until we are 15ms away from our target.
	// This guarantees Windows wakes up before the 33ms mark.
	auto sleep_target = end_time - std::chrono::milliseconds(15);
	if (std::chrono::steady_clock::now() < sleep_target)
	{
		std::this_thread::sleep_until(sleep_target);
	}

	// Burn the remaining ~15 milliseconds precisely
	while (std::chrono::steady_clock::now() < end_time)
	{
		// Do nothing
	}

}

void CPong::draw()
{

	_data_mutex.lock();

	// Clear the canvas
	_canvas.setTo(cv::Scalar(0, 0, 0));

	// Draw the GUI Window Panel
	// Format: cvui::window(canvas, x, y, width, height, "Title");
	std::string windowTitle = "Pong (FPS: " + std::to_string((int)_fps) + ")";
	cvui::window(_canvas, 10, 10, 220, 250, windowTitle);

	// --- SCORE DISPLAY ---
	std::string scoreText = "Player: " + std::to_string(_score_player) + "   Computer: " + std::to_string(_score_computer);
	cvui::text(_canvas, 20, 40, scoreText);

	// --- BALL RADIUS TRACKBAR --- (5 to 100) 
	cvui::text(_canvas, 25, 70, "Ball Radius");
	cvui::trackbar(_canvas, 15, 80, 180, &_ball_radius, 5, 100);

	// --- BALL SPEED TRACKBAR --- (100 to 400) 
	cvui::text(_canvas, 25, 140, "Ball Speed");
	cvui::trackbar(_canvas, 15, 160, 180, &_ball_velocity_magnitude, 100, 400);

	// --- RESET BUTTON ---
	if (cvui::button(_canvas, 20, 220, "Reset"))
	{
		_reset_pending = true;
		_score_computer = 0;
		_score_player = 0;
		_ball_position = cv::Point2f(_canvas.cols / 2.0f, _canvas.rows / 2.0f);
		_game_over = false;
		_last_tick = cv::getTickCount();
	}


	// --- GAME OBJECTS ---
	if (_game_over == false)
	{
		cv::circle(_canvas, _ball_position, _ball_radius, cv::Scalar(255, 255, 255), -1);
		cv::rectangle(_canvas, _player_paddle, cv::Scalar(200, 200, 200), -1);
		cv::rectangle(_canvas, _computer_paddle, cv::Scalar(200, 200, 200), -1);
	}

	// --- GAME OVER DISPLAY ---
	if (_game_over == true)
	{
		std::string winnerText = (_score_player >= 5) ? "PLAYER WINS!" : "COMPUTER WINS!";
		cv::putText(_canvas, winnerText, cv::Point(_canvas.cols / 2 - 170, _canvas.rows / 2 - 50),
			cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 255, 0), 3);
		cv::putText(_canvas, "Press Reset in GUI to play again", cv::Point(_canvas.cols / 2 - 170, _canvas.rows / 2 + 30),
			cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
	}

	cvui::update();
	_data_mutex.unlock();
	cv::imshow("Pong", _canvas);
	cv::waitKey(1);
}


void CPong::gpio_thread(CPong* ptr)
{
	while (ptr->_thread_exit == false)
	{
		ptr->gpio();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void CPong::update_thread(CPong* ptr)
{
	while (ptr->_thread_exit == false)
	{
		ptr->update();
	}
}

void CPong::draw_thread(CPong* ptr)
{
	cvui::init("Pong");
	while (ptr->_thread_exit == false)
	{
		ptr->draw();
	}
}


void CPong::run()
{
	_thread_exit = false;

	std::thread t1(&CPong::gpio_thread, this);
	std::thread t2(&CPong::update_thread, this);
	std::thread t3(&CPong::draw_thread, this);

	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	t1.join();
	t2.join();
	t3.join();
}