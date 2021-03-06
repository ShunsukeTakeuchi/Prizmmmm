#pragma once

#include<string>
#include<Windows.h>

/*
Mouse and touch input has raw input data.
These need initialized in WinAPI's event handler.
*/

namespace Prizm
{
	using KeyCode = unsigned int;
	
	namespace Input
	{
		// multi touch num
		constexpr int max_touchcount = 2;

		void Initialize(void);

		// mouse capture
		void CaptureMouse(HWND, bool do_capture);
		bool IsMouseCaptured(void);
		POINT MouseCapturePosition(void);

		// update
		void KeyDown(KeyCode);
		void KeyUp(KeyCode);

		void ButtonDown(KeyCode);
		void ButtonUp(KeyCode);
		void UpdateMousePos(long, long, short);

		void UpdateTouchPos(long, long, int, DWORD);

		// key state
		bool IsKeyPress(const char*);
		
		bool IsKeyReleased(const char*);
		
		bool IsKeyTriggered(const char*);

		// mouse state
		bool IsMousePress(const char*);
		
		bool IsScrollUp(void);
		bool IsScrollDown(void);

		int MouseDeltaX(void);
		int MouseDeltaY(void);

		// touch state
		bool IsTouchPress(int);
		bool IsTouchMove(int);
		bool IsTouchReleased(int);
		bool IsTouchTriggered(int);

		int TouchDeltaX(int);
		int TouchDeltaY(int);

		const long* GetMouseDelta(void);
		const long* GetTouchDelta(int);

		// update end of frame
		void PostStateUpdate(void);
	};
}