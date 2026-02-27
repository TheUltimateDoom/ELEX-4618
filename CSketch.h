#pragma once
#include "CBase4618.h"
#include <vector>


#define BTN_COLOR_CH  S1
#define BTN_RESET_CH  S2

/**
 * @brief A class that implements the Etch-A-Sketch game with hardware integration.
 * Inherits from CBase4618 and provides hardware I/O, game logic, and rendering
 * for a drawing application controlled by joysticks, buttons, and an accelerometer.
 * 
 * @author Farhan Sadiq
 */
class CSketch : public CBase4618
{
private:
	int _colorIndex; ///< Current color index in the _colors vector
	bool _resetPending; ///< Flag to indicate if a reset is pending (set in update, handled in draw)
	cv::Point _currentPos; ///< Current position of the "pen" on the canvas
	cv::Point _lastPos; ///< Last position of the "pen" (used for drawing lines)

	int _dataX; ///< Joystick X-axis data
	int _dataY; ///< Joystick Y-axis data

	int _btnColorRaw; ///< Raw state of the color change button (active low)
	int _btnResetRaw; ///< Raw state of the reset button (active low)

	int _lastBtnColorState; ///< Last state of the color button for edge detection
	int _lastBtnResetState; ///< Last state of the reset button for edge detection
	double _lastColorTime; ///< Timestamp of the last color change for debouncing

	cv::Mat _drawing; ///< Persistent layer for drawn lines (separate from _canvas which is cleared each frame)
	std::vector<cv::Scalar> _colors; ///< List of colors available for drawing (in BGR format)
	std::vector<std::string> _colorNames; ///< Corresponding color names to display

	cv::Rect _btnResetRect; ///< Rectangle defining the area of the Reset button for mouse interaction
	cv::Rect _btnExitRect; ///< Rectangle defining the area of the Exit button for mouse interaction

	float _accelX; ///< Current X-axis acceleration
	float _accelY; ///< Current Y-axis acceleration
	float _accelZ; ///< Current Z-axis acceleration
	float _prevAccelX; ///< Previous X-axis acceleration (used for shake detection)	
	float _prevAccelY; ///< Previous Y-axis acceleration (used for shake detection)
	float _prevAccelZ; ///< Previous Z-axis acceleration (used for shake detection)

	/**
	 * @brief Static callback function for OpenCV mouse events.
	 * @param event The type of mouse event (e.g., LBUTTONDOWN).
	 * @param x The x-coordinate of the mouse.
	 * @param y The y-coordinate of the mouse.
	 * @param flags Specific event flags.
	 * @param userdata Pointer to the CSketch instance (this).
	 */
    static void on_mouse(int event, int x, int y, int flags, void* userdata);

public:
	/**
	 * @brief Constructor for the CSketch class.
	 * @param size The desired dimensions of the drawing canvas.
	 * @param comPort The COM port number for the microcontroller communication.
	 */
    CSketch(cv::Size size, int comPort);

	/**
	 * @brief Handles all hardware communication.
	 * Reads joystick, accelerometer, and buttons; writes to hardware LEDs.
	 */
    virtual void gpio() override;

	/**
	 * @brief Implements game logic and physics.
	 * Calculates velocity-based movement, handles button debouncing, and detects shaking.
	 */
    virtual void update() override;

	/**
	 * @brief Renders the game state to the screen.
	 * Clears the canvas, overlays the drawing layer, and renders UI text/buttons.
	 */
    virtual void draw() override;
};