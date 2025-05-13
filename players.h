#pragma once

#include "color.h"
#include <opencv2/core/types.hpp>

using namespace std;
using namespace cv;

class Player
{
	public:
		Color color;
		Point position;
		Rect rect;
		Player();
		Player(Color color, Rect rect, int x, int y) {
			this->position = Point(x,y);
			this->color = color;
			this->rect = rect;
		}

};
