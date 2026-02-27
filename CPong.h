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
	cv::Point2f _ball_position;   ///< Current (x,y) position of the ball
	cv::Point2f _ball_velocity;   ///< Current X and Y velocity of the ball in pixels/sec
	int _ball_radius;             ///< Radius of the ball (adjustable via GUI 5-100)
	int _ball_velocity_magnitude; ///< Base speed (adjustable via GUI 100-400)
	cv::Rect _player_paddle;      ///< Player paddle (controlled by joystick)
	cv::Rect _computer_paddle;    ///< Computer paddle (auto-controlled)

	// --- Game State ---
	int _score_player;            ///< Player's current score
	int _score_computer;          ///< Computer's current score
	bool _game_over;              ///< Flag to end the game (player/computer has scored 5 or reset button was pressed)
	bool _reset_pending;          ///< Flag to trigger a game reset

	// --- Timing & Physics ---
	double _last_tick;            ///< OpenCV tick count from the previous frame for delta time
	double _fps;                  ///< Calculated FPS

	// --- Hardware Data ---
	float _joyY;                  ///< Joystick Y-axis percentage (0-100)
	int _button_reset;            ///< Raw digital state of the physical reset button
	int _button_reset_previous;   ///< Previous state to detect a button press (falling edge)

	// --- Multi-threading ---
	bool _thread_exit;            ///< Flag to tell threads when to exit
	std::mutex _data_mutex;       ///< Mutex to thread-safe shared variables

	/**
	 * @brief Static thread wrapper required by C++ to run class methods in threads
	 */
	static void gpio_thread(CPong* ptr);

	/**
	 * @brief Static thread wrapper required by C++ to run class methods in threads
	 */
	static void update_thread(CPong* ptr);

	/**
	 * @brief Static thread wrapper required by C++ to run class methods in threads
	 */
	static void draw_thread(CPong* ptr);

public:
	/**
	 * @brief Constructor for the CPong class.
	 * @param size Dimensions of the game canvas.
	 * @param comPort COM port for the microcontroller.
	 */
	CPong(cv::Size size, int comPort);

	/**
	 * @brief Runs the game.
	 * Calls gpio, update, and draw in a loop until the user presses 'q' or the game ends.
	 * It also manages thread creation and cleanup for the gpio, update, and draw methods.
	 */
	void run();

	/**
	 * @brief Reads hardware inputs from the microcontroller.
	 * In particular, it reads analog vertical joystick, and the S1 pushbutton.
	 */
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