/*
 PlayState.h
 LD19 Entry
 
 Created by Chad Godsey on 12/18/10.
 
 Copyright 2009-10 BlitThis! studios.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Gosu/Gosu.hpp>
#include <MUGE.h>

#define MAX_AI 64

struct ai
{
	SpriteSheet walk, run;
	b2Vec2 pos;
	double speed;
	bool walking;
};


class Core;

/**
 * State for
 */
class PlayState : public GameState
{
public:
	PlayState( std::wstring _config );
	
	void init();
	void cleanup();
	
	void pause();
	void resume();
	
	void update();
	void draw() const;
	
	void save();
	
private:

	bool testPoint(int i);

	std::wstring m_ConfigFile;

	Gosu::Font *m_Font;
	SceneGraph m_Graph;

	Sprite m_hud, m_hud2;
	
	Gosu::Color m_canvasColor;

	Camera_Parallax m_Camera;
	RenderManager m_rendMan;
	AudioManager m_audMan;

	int m_AnimLock;
	int m_Action;
	SpriteSheet m_Idle, m_Run, m_LPunch, m_RPunch, m_HButt, m_Bystand;

	bool m_Left;

	//Player data bs
	b2Vec2 m_PlayerPos;
	b2Vec2 m_AIPos;
	//MyPlayer m_Player;

	ai m_AIs[MAX_AI];
	//a shape to test collision with
	b2PolygonShape m_Shape;

	int m_Score;
	int m_Continuous;

	//	Pixel transformation data
	// Focus is the level coordinates of the center of the screen
	// Zoom is a zooming factor for all layers
	// Scale is the x/y scale to transform from level coordinates to screen
	// Width and Height are screen size
	double m_Focus[2], m_Offset[2], m_Extents[2];
	double m_Zoom;
	double m_Rot;
	int m_Scale;
	int m_Width, m_Height;
};