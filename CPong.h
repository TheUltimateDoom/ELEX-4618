#pragma once
#include "CBase4618.h"

/**
 * @brief Implements the classic arcade game Pong.
 *
 * This class inherits from CBase4618.
 */
class CPong : public CBase4618
{
private:
	// --- Game Objects ---
	cv::Point2f _ballPos;      ///< Current (x,y) position of the ball
	cv::Point2f _ballVelocity; ///< Current X and Y velocity of the ball in pixels/sec
	int _ballRadius;           ///< Radius of the ball (adjustable via GUI 5-100)
	int _ballSpeedMag;         ///< Base speed magnitude (adjustable via GUI 100-400)
	cv::Rect _playerPaddle;    ///< Player paddle (controlled by joystick)
	cv::Rect _computerPaddle;  ///< Computer paddle (auto-controlled)

	// --- Game State ---
	int _scorePlayer;          ///< Player's current score
	int _scoreComputer;        ///< Computer's current score
	bool _gameOver;            ///< Flag to end the game (player/computer has scored 5 or reset button was pressed)
	bool _resetPending;        ///< Flag to trigger a game reset

	// --- Timing & Physics ---
	double _lastTick;          ///< OpenCV tick count from the previous frame for delta time
	double _fps;               ///< Calculated FPS

	// --- Hardware Data ---
	float _joyY;               ///< Joystick Y-axis percentage (0-100)
	int _btnResetRaw;          ///< Raw digital state of the physical reset button
	int _lastBtnResetState;    ///< Previous state to detect a button press (falling edge)

	// --- Multi-threading ---
	//std::mutex _data_mutex; ///< Mutex to thread-safe shared variables
	//bool _thread_exit;      ///< Flag to tell threads when to exit

	/**
	 * @brief Static thread wrappers required by C++ to run class methods in threads
	 */
	//static void update_gpio_thread(CPong* ptr);
	//static void update_logic_thread(CPong* ptr);
	//static void draw_thread_wrapper(CPong* ptr);

public:
	/**
	 * @brief Constructor for the CPong class.
	 * @param size Dimensions of the game canvas.
	 * @param comPort COM port for the microcontroller.
	 */
	CPong(cv::Size size, int comPort);


	//void run();

	/**
	 * @brief Reads hardware inputs from the microcontroller.
	 * In particular, it reads analog vertical joystick, and the S1 pushbutton.
]	 */
	virtual void gpio() override;

	/**
	 * @brief Computes the game logic, physics, and frame timing.
	 * This method calculates dt to ensure the ball moves at a
	 * consistent speed independent of the CPU rate. It computes the new
	 * positions for the ball and paddles, checks for collisions with walls or paddles,
	 * and updates the score. It also enforces the game over state at 5 points
	 * and utilizes thread sleeping to attempt to lock the system update rate to 30 FPS.
	 */
	virtual void update() override;

	/**
	 * @brief Renders the game state and GUI to the screen.
	 * Clears the OpenCV canvas and draws the current frame. This includes
	 * rendering the ball, both paddles, and the current score. It also
	 * processes and draws the cvui graphical interface elements, including the
	 * FPS counter, ball radius and speed trackbars, and the Reset/Exit buttons.
	 */
	virtual void draw() override;
};