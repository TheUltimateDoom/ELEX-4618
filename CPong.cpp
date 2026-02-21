#include "stdafx.h"
#include "CPong.h"

#include "cvui.h"
#include <chrono>
#include <thread>
#include <mutex>


CPong::CPong(cv::Size size, int comPort)
{
	// Initialize serial communication
	_control.init_com(comPort);

	// Create the canvas
	_canvas = cv::Mat::zeros(size, CV_8UC3);

	// Initialize CVUI for the GUI elements
	cvui::init("Pong");

	// Initialize Game State
	_scorePlayer = 0;
	_scoreComputer = 0;
	_gameOver = false;
	_resetPending = false;

	// Initialize Timing
	_lastTick = cv::getTickCount();
	_fps = 0.0;

	// Initialize Hardware Data
	_joyY = 50.0f;
	_btnResetRaw = 1;
	_lastBtnResetState = 1;

	// Initialize GUI Settings to Defaults
	_ballRadius = 20;     // Default radius
	_ballSpeedMag = 250;  // Default speed (pixels per second)

	// Setup initial positions
	_ballPos = cv::Point2f(size.width / 2.0f, size.height / 2.0f);

	// Give the ball an initial direction (e.g., moving down-right)
	_ballVelocity = cv::Point2f(_ballSpeedMag * 0.707f, _ballSpeedMag * 0.707f);

	// Setup Paddles (Width, Height)
	int paddleW = 20;
	int paddleH = 100;
	_playerPaddle = cv::Rect(size.width - paddleW - 20, size.height / 2 - paddleH / 2, paddleW, paddleH);
	_computerPaddle = cv::Rect(20, size.height / 2 - paddleH / 2, paddleW, paddleH);
}


void CPong::gpio()
{
	_joyY = _control.get_analog(JOY_Y);
	_control.get_data(DIGITAL, S1, _btnResetRaw);
}
void CPong::update()
{
	auto end_time = std::chrono::system_clock::now() + std::chrono::milliseconds(33);

	// Detect falling edge (button was just pressed down)
	if (_btnResetRaw == 0 && _lastBtnResetState == 1)
	{
		// Triggers the ball to center
		_resetPending = true;

		// Resets scores and unfreezes the game
		_scorePlayer = 0;
		_scoreComputer = 0;
		_gameOver = false;
	}
	// Store the current button state for the next frame to detect edges
	_lastBtnResetState = _btnResetRaw;


	// Check if either player has reached 5 points to end the game
	if (_scorePlayer >= 5 || _scoreComputer >= 5)
	{
		_gameOver = true;
	}

	// Handle pending reset after a score
	if (_resetPending)
	{
		// Put ball back in the middle
		_ballPos = cv::Point2f(_canvas.cols / 2.0f, _canvas.rows / 2.0f);

		// Pick a new random direction
		_ballVelocity = cv::Point2f(_ballSpeedMag * 0.707f, _ballSpeedMag * 0.707f);

		_resetPending = false;
	}


	if (_gameOver == false)
	{
		// Calculate dt
		double currentTick = cv::getTickCount();
		double dt = (currentTick - _lastTick) / cv::getTickFrequency();
		_lastTick = currentTick;

		// Calculates FPS
		if (dt > 0)
		{
			_fps = 1.0 / dt;
		}

		float currentSpeed = std::sqrt((_ballVelocity.x * _ballVelocity.x) + (_ballVelocity.y * _ballVelocity.y));

		if (currentSpeed > 0)
		{
			// Normalize the vector and multiply by the GUI trackbar value
			_ballVelocity.x = (_ballVelocity.x / currentSpeed) * _ballSpeedMag;
			_ballVelocity.y = (_ballVelocity.y / currentSpeed) * _ballSpeedMag;
		}

		// Move the Ball 
		_ballPos.x += _ballVelocity.x * dt;
		_ballPos.y += _ballVelocity.y * dt;

		// Simple Wall Collision (Bounce off top and bottom) 

		// Top Wall Bounce
		if (_ballPos.y - _ballRadius < 0)
		{
			_ballPos.y = _ballRadius;         // Snap to edge to prevent getting stuck
			_ballVelocity.y = std::abs(_ballVelocity.y); // Force velocity downwards
		}
		// Bottom Wall Bounce
		else if (_ballPos.y + _ballRadius > _canvas.rows)
		{
			_ballPos.y = _canvas.rows - _ballRadius; // Snap to edge
			_ballVelocity.y = -std::abs(_ballVelocity.y); // Force velocity upwards
		}

		// --- SCORING (Left and Right Walls) ---
		// Right Wall (Computer Scores)
		if (_ballPos.x + _ballRadius > _canvas.cols)
		{
			_scoreComputer++;
			_resetPending = true; // Flag for a reset
		}
		// Left Wall (Player Scores)
		else if (_ballPos.x - _ballRadius < 0)
		{
			_scorePlayer++;
			_resetPending = true; // Flag for a reset
		}

		// PADDLE COLLISIONS ---

		// Player Paddle Collision (Right side)
		// 1. Check if the ball's bounding box overlaps the paddle's rectangle
		if (_ballPos.x + _ballRadius >= _playerPaddle.x &&
			_ballPos.x - _ballRadius <= _playerPaddle.x + _playerPaddle.width &&
			_ballPos.y + _ballRadius >= _playerPaddle.y &&
			_ballPos.y - _ballRadius <= _playerPaddle.y + _playerPaddle.height)
		{
			// Snap ball to the edge of the paddle to prevent it from getting stuck inside
			_ballPos.x = _playerPaddle.x - _ballRadius;

			// Calculate exactly where the ball hit the paddle (relative to the paddle's center)
			float paddleCenterY = _playerPaddle.y + (_playerPaddle.height / 2.0f);
			float relativeIntersectY = paddleCenterY - _ballPos.y;

			// Normalize this value from -1.0 (bottom edge) to 1.0 (top edge)
			float normalizedIntersectY = relativeIntersectY / (_playerPaddle.height / 2.0f);

			// Calculate the bounce angle (Max bounce angle is 45 degrees, which is ~0.785 radians)
			float bounceAngle = normalizedIntersectY * 0.785f;

			// Apply the new velocity using Trigonometry!
			// X is negative because it's bouncing left, away from the player
			_ballVelocity.x = -_ballSpeedMag * cos(bounceAngle);

			// Y is negative sine because OpenCV's Y-axis is inverted (0 is at the top)
			_ballVelocity.y = _ballSpeedMag * -sin(bounceAngle);
		}

		// Computer Paddle Collision (Left side)
		if (_ballPos.x - _ballRadius <= _computerPaddle.x + _computerPaddle.width &&
			_ballPos.x + _ballRadius >= _computerPaddle.x &&
			_ballPos.y + _ballRadius >= _computerPaddle.y &&
			_ballPos.y - _ballRadius <= _computerPaddle.y + _computerPaddle.height)
		{
			// Snap ball to the edge of the paddle
			_ballPos.x = _computerPaddle.x + _computerPaddle.width + _ballRadius;

			float paddleCenterY = _computerPaddle.y + (_computerPaddle.height / 2.0f);
			float relativeIntersectY = paddleCenterY - _ballPos.y;

			float normalizedIntersectY = relativeIntersectY / (_computerPaddle.height / 2.0f);
			float bounceAngle = normalizedIntersectY * 0.785f;

			// X is positive because it's bouncing right, away from the computer
			_ballVelocity.x = _ballSpeedMag * cos(bounceAngle);
			_ballVelocity.y = _ballSpeedMag * -sin(bounceAngle);
		}

		// 5. Update Paddles
		// Player Paddle: Map the 0-100 joystick percentage to the screen height
		_playerPaddle.y = (_joyY / 100.0f) * (_canvas.rows - _playerPaddle.height);

		// Computer Paddle: A simple AI that perfectly tracks the ball's Y position
		_computerPaddle.y = _ballPos.y - (_computerPaddle.height / 2);

		// Clamp paddles to screen so they don't disappear
		if (_playerPaddle.y < 0) _playerPaddle.y = 0;
		if (_playerPaddle.y > _canvas.rows - _playerPaddle.height) _playerPaddle.y = _canvas.rows - _playerPaddle.height;
		if (_computerPaddle.y < 0) _computerPaddle.y = 0;
		if (_computerPaddle.y > _canvas.rows - _computerPaddle.height) _computerPaddle.y = _canvas.rows - _computerPaddle.height;
	}

	std::this_thread::sleep_until(end_time);
}

void CPong::draw()
{
	// 1. Clear the canvas
	_canvas = cv::Mat::zeros(_canvas.size(), CV_8UC3);

	// 2. Calculate FPS for the display 
	double currentTick = cv::getTickCount();
	_fps = cv::getTickFrequency() / (currentTick - _lastTick);
	_lastTick = currentTick;

	// 3. Draw the GUI Window Panel
	// Format: cvui::window(canvas, x, y, width, height, "Title"); [cite: 322]
	std::string windowTitle = "Pong (FPS: " + std::to_string((int)_fps) + ")";
	cvui::window(_canvas, 10, 10, 220, 250, windowTitle);

	// --- ADD THIS: The Score Display ---
	std::string scoreText = "Player: " + std::to_string(_scorePlayer) + "   Computer: " + std::to_string(_scoreComputer);
	cvui::text(_canvas, 20, 40, scoreText);
	// (Make sure to shift your trackbar Y-coordinates down a bit so they don't overlap the text!)

	// 4. Ball Radius Trackbar (5 to 100) 
	cvui::text(_canvas, 25, 70, "Ball Radius");
	cvui::trackbar(_canvas, 15, 80, 180, &_ballRadius, 5, 100);

	// 5. Ball Speed Trackbar (100 to 400) 
	cvui::text(_canvas, 25, 140, "Ball Speed");
	cvui::trackbar(_canvas, 15, 160, 180, &_ballSpeedMag, 100, 400);

	// 6. Reset and Exit Buttons 
	if (cvui::button(_canvas, 20, 220, "Reset"))
	{
		_resetPending = true;
		_scoreComputer = 0;
		_scorePlayer = 0;
		_gameOver = false;
	}

	// --- DRAW GAME OBJECTS ---
	// Draw the Ball (using the GUI trackbar radius)
	cv::circle(_canvas, _ballPos, _ballRadius, cv::Scalar(255, 255, 255), -1);

	// Draw Player Paddle (Right)
	cv::rectangle(_canvas, _playerPaddle, cv::Scalar(200, 200, 200), -1);

	// Draw Computer Paddle (Left)
	cv::rectangle(_canvas, _computerPaddle, cv::Scalar(200, 200, 200), -1);

	// --- ADD THIS: Game Over Text ---
	if (_gameOver == true)
	{
		std::string winnerText = (_scorePlayer >= 5) ? "PLAYER WINS!" : "COMPUTER WINS!";

		// Draw huge text in the middle of the screen
		cv::putText(_canvas, winnerText, cv::Point(_canvas.cols / 2 - 150, _canvas.rows / 2 - 50),
			cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 255, 0), 3);
		cv::putText(_canvas, "Press Reset in GUI to play again", cv::Point(_canvas.cols / 2 - 170, _canvas.rows / 2 + 20),
			cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
	}

	// Update CVUI and Show Image 
	cvui::update();
	cv::imshow("Pong", _canvas);
}