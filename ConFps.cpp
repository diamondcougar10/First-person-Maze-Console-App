// ConFps.cpp : This file contains the 'main' function. Program execution begins and ends there. by Ryan

#include <iostream>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <wchar.h>


using namespace std;

int ScreenWidth = 120;
int ScreenHeight = 40;
float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f; // angle player is looking at
int MapHeight = 16;
int MapWidth = 16;
float fov = 3.14159 / 4.0;
float Depth = 16.0f;


int main()
{
	// create screen buffer

	wchar_t* screen = new wchar_t[ScreenWidth * ScreenHeight];
	HANDLE Console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(Console);
	DWORD dwBytesWritten;
	wstring map;



	map += L"################";
	map += L"#      #       #";
	map += L"#              #";
	map += L"#      # #     #";
	map += L"#      #       #";
	map += L"#      #       #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#              #";
	map += L"#        #######";
	map += L"#              #";
	map += L"#              #";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	// draw the game loop here

	while (1)
	{
		tp2 = chrono::system_clock::now();
		chrono::duration<float> time_span = chrono::duration_cast<chrono::duration<float>>(tp2 - tp1);
		tp1 = tp2;
		float ElapsedTime = time_span.count();

		//controlls
		//handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= 0.8f * ElapsedTime;
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += 0.8f * ElapsedTime;

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerA) * ElapsedTime * 5.0f; // move player forward
			fPlayerY += cosf(fPlayerA) * ElapsedTime * 5.0f; // move player forward

			// collision detection with map
			if (map[(int)fPlayerY * MapWidth + (int)fPlayerX] == '#') 
			{
				fPlayerX -= sinf(fPlayerA) * ElapsedTime * 5.0f; // move player backward
				fPlayerY -= cosf(fPlayerA) * ElapsedTime * 5.0f; // move player backward
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerA) * ElapsedTime * 5.0f; // move player backward
			fPlayerY -= cosf(fPlayerA) * ElapsedTime * 5.0f; // move player backward

			if (map[(int)fPlayerY * MapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX += sinf(fPlayerA) * ElapsedTime * 5.0f; // move player forward
				fPlayerY += cosf(fPlayerA) * ElapsedTime * 5.0f; // move player forward
			}
		}

		for (int x = 0; x < ScreenWidth; x++)
		{
			float rayAngle = (fPlayerA - fov / 2.0f) + ((float)x / (float)ScreenWidth) * fov;

			float rayDistance = 0;
			bool HitWall = false;
			bool Boundary = false;

			float EyeX = sinf(rayAngle); //unit vector for the ray in the direction of the player's view direction'
			float EyeY = cosf(rayAngle);

			while (!HitWall && rayDistance < Depth)
			{
				rayDistance += 0.1f;

				int TestX = (int)(fPlayerX + EyeX * rayDistance); // test the ray for collision with a wall
				int TestY = (int)(fPlayerY + EyeY * rayDistance);

				//test if ray is out of bounds

				if (TestX < 0 || TestX >= MapWidth || TestY < 0 || TestY >= MapHeight)
				{
					HitWall = true;
					rayDistance = Depth; // clamp to maximum depth
				}
				else
				{
					// Ray is inbounds so test to see if ray is hitting a wall
					if (map[TestY * MapWidth + TestX] == '#')
					{
						HitWall = true;

						vector<pair<float, float>> p; // distances...Dot?
						for(int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++) 
							{
								float vy = TestY + ty - fPlayerY;
								float vx = TestX + tx - fPlayerX;
								float d = sqrtf(vx * vx + vy * vy);
								float dot = (EyeX * vx / d) + (EyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}

						sort(p.begin(), p.end(), [](const pair<float, float> &left , const pair<float, float> &right) { return left.first < right.first; });

						float Bound = 0.01;
						if (acos(p.at(0).second) < Bound) Boundary = true;
						if (acos(p.at(1).second) < Bound) Boundary = true;
						if (acos(p.at(2).second) < Bound) Boundary = true;

					}
				}
			}

			// Calculate distence to ceiling and floor
			int Ceiling = (float)ScreenHeight / 2.0f - ScreenHeight / ((float)rayDistance);
			int Floor = ScreenHeight - Ceiling;

			short Shade = ' ';

			if (rayDistance <= Depth / 4.0f) Shade = 0x2588;
			else if (rayDistance <= Depth / 3.0f) Shade = 0x2593;
			else if (rayDistance <= Depth / 2.0f) Shade = 0x2592;
			else if (rayDistance <= Depth) Shade = 0x2591;
			else Shade = ' ';
			if (Boundary) Shade = ' '; // Add boundary marker if ray hits a boundary

			for (int y = 0; y < ScreenHeight; y++)
			{
				if (y < Ceiling)
					screen[y * ScreenWidth + x] = ' '; // Sky
				else if (y > Ceiling && y <= Floor)
					screen[y * ScreenWidth + x] = Shade; // Walls
				else
				{
					// Shade the floor based on ray distance
					float b = 1.0f - (((float)y - ScreenHeight / 2.0f) / ((float)ScreenHeight / 2.0f));
					char FloorShade; // Use a separate variable for the floor
					if (b < 0.25) FloorShade = '#';
					else if (b < 0.5) FloorShade = 'x';
					else if (b < 0.75) FloorShade = '.';
					else if (b < 0.9) FloorShade = '-';
					else FloorShade = ' ';
					screen[y * ScreenWidth + x] = FloorShade;
				}
			}



		}
		// display stats
		// Display Stats
		swprintf_s(screen, ScreenWidth, L"FPS: %.2f, Player X: %.2f, Y: %.2f, A: %.2f",
			1.0f / ElapsedTime, fPlayerX, fPlayerY, fPlayerA);

		// Display Map (start below the stats row, e.g., row 2)
		for (int ny = 0; ny < MapHeight; ny++) {
			for (int nx = 0; nx < MapWidth; nx++) {
				screen[(ny + 2) * ScreenWidth + nx] = map[ny * MapWidth + nx]; // Start at row 2
			}
		}

		// Draw Player on Map
		screen[((int)fPlayerY + 2) * ScreenWidth + (int)fPlayerX] = 'P'; // Offset by 2 rows



		screen[ScreenWidth * ScreenHeight - 1] = '\0'; // ensure null termination
		WriteConsoleOutputCharacter(Console, screen, ScreenWidth * ScreenHeight, { 0, 0 }, &dwBytesWritten); // write blank screen

	}

	return 0;
}
