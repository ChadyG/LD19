/*
 PlayState.cpp
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

#include <boost/lexical_cast.hpp>

#include "PlayState.h"
#include "Core/Core.h"

/**
 *
 */

PlayState::PlayState( std::wstring _config )
{
	m_ConfigFile = _config;
}

void PlayState::init()
{	
	m_Engine = Core::getCurrentContext();
	m_Engine->showCursor( false );

	m_Font = &m_rendMan.font();
	m_hud.setImage(new Gosu::Image(Core::getCurrentContext()->graphics(), 
		Gosu::sharedResourcePrefix() + L"Images/HUD1.png"));
	m_hud.setLayer(10.0);
	m_hud2.setImage(new Gosu::Image(Core::getCurrentContext()->graphics(), 
		Gosu::sharedResourcePrefix() + L"Images/HUD2.png"));
	m_hud2.setLayer(10.0);

	m_AnimLock = 0;

	m_Score = 0;
	m_Continuous = 0;

	m_Shape.SetAsBox(3.2f, 6.4f);

	m_Width = m_Engine->graphics().width();
	m_Height = m_Engine->graphics().height();

	m_Zoom = 0.70;
	m_Rot = 0.0;

	//Register camera to managers
	m_rendMan.setCamera( &m_Camera );
	m_audMan.setCamera( &m_Camera );
	InputManager::getCurrentContext()->setCamera( &m_Camera );

	SceneGraph::setCurrentContext( &m_Graph );
	RenderManager::setCurrentContext( &m_rendMan ); 
	AudioManager::setCurrentContext( &m_audMan );

	Json::Value jVal;
	Json::Reader reader;
	reader.parseFile( Gosu::narrow(Gosu::resourcePrefix() + L"Data/" + m_ConfigFile + L".json"), jVal);
	
	m_PlayerPos.Set(
		(float32)jVal["PlayerSpawn"].get(0u, 0.0).asDouble(), 
		(float32)jVal["PlayerSpawn"].get(1u, 0.0).asDouble());

	// Background color
	m_canvasColor = Gosu::Color( 
		jVal["CanvasColor"].get(0u, 255).asInt(),
		jVal["CanvasColor"].get(1u, 255).asInt(),
		jVal["CanvasColor"].get(2u, 255).asInt(),
		jVal["CanvasColor"].get(3u, 255).asInt());
	
	// Coordinate transform stuff for world -> screen
	m_Scale = jVal.get("Scale", 1).asInt();
	
	// Configure camera for screen transformation
	m_Camera.setExtents( m_Width, m_Height);
	m_Camera.setScale( m_Scale );

	// Configure camera stuff
	// Set up layer scales
	int i;
	for (i = 0; i < jVal["Layers"].size(); ++i) {
		m_Camera.addLayer(jVal["Layers"][i]["Layer"].asInt(), jVal["Layers"][i]["Scale"].asDouble());
	}
	m_Focus[0] = m_PlayerPos.x;
	m_Focus[1] = m_PlayerPos.y - 4.0;
	
	m_rendMan.setCamera( m_Focus[0], m_Focus[1], m_Zoom, m_Rot);
	
	// Set screen offset from world focus point
	//CameraTransform camtrans = m_Camera.worldToScreen(m_Focus[0], m_Focus[1], 1);
	//m_Offset[0] = camtrans.x;
	//m_Offset[1] = camtrans.y;
	
	// Create all scene objects from file
	m_Graph.loadFile( m_ConfigFile );

	m_Left = false;
	m_Idle.setImage(Gosu::sharedResourcePrefix() + L"Images/robot_idle.png", 90, 128, 15);
	m_Run.setImage(Gosu::sharedResourcePrefix() + L"Images/robot_roll.png", 90, 128, 15);
	m_LPunch.setImage(Gosu::sharedResourcePrefix() + L"Images/robot_leftpunch.png", 102, 128, 4);
	m_RPunch.setImage(Gosu::sharedResourcePrefix() + L"Images/robot_rightpunch.png", 102, 128, 4);
	m_HButt.setImage(Gosu::sharedResourcePrefix() + L"Images/robot_headbutt.png", 102, 128, 4);

	m_Idle.setVisible(false);
	m_Idle.setX(m_PlayerPos.x);
	m_Idle.setY(m_PlayerPos.y);
	
	m_Run.setVisible(true);
	m_Run.setX(m_PlayerPos.x);
	m_Run.setY(m_PlayerPos.y);

	m_LPunch.setVisible(false);
	m_LPunch.setX(m_PlayerPos.x);
	m_LPunch.setY(m_PlayerPos.y);

	m_RPunch.setVisible(false);
	m_RPunch.setX(m_PlayerPos.x);
	m_RPunch.setY(m_PlayerPos.y);

	m_HButt.setVisible(false);
	m_HButt.setX(m_PlayerPos.x);
	m_HButt.setY(m_PlayerPos.y);

	m_Bystand.setVisible(true);
	m_Bystand.setX(m_AIPos.x);
	m_Bystand.setY(m_AIPos.y);
	
	m_rendMan.registerRenderable(0, &m_Idle);
	m_rendMan.registerRenderable(0, &m_Run);
	m_rendMan.registerRenderable(0, &m_LPunch);
	m_rendMan.registerRenderable(0, &m_RPunch);
	m_rendMan.registerRenderable(0, &m_HButt);

	for (int i=0; i<MAX_AI; ++i) {
		m_AIs[i].walking = true;
		m_AIs[i].speed = Gosu::random(0.05, 0.15);
		m_AIs[i].pos.Set(20.0 + i*15.0 + Gosu::random(0.0, 50.0), 0.0);
		if (Gosu::random(0.0,2.0) > 1.0) {
			m_AIs[i].walk.setImage(Gosu::sharedResourcePrefix() + L"Images/bystander.png", 64, 128, 20);
			m_AIs[i].run.setImage(Gosu::sharedResourcePrefix() + L"Images/bystander_run.png", 64, 135, 20);
		}else{
			m_AIs[i].walk.setImage(Gosu::sharedResourcePrefix() + L"Images/bystander2.png", 64, 128, 20);
			m_AIs[i].run.setImage(Gosu::sharedResourcePrefix() + L"Images/bystander_run2.png", 64, 135, 20);
		}
		m_AIs[i].run.setVisible(false);
		m_AIs[i].walk.setCenter(0.5, 1.0);
		m_AIs[i].run.setCenter(0.5, 1.0);
		
		m_AIs[i].walk.setSpeed((int)(m_AIs[i].speed*20));
		m_AIs[i].run.setSpeed((int)(m_AIs[i].speed*20));
		//this is a terrible idea
		Gosu::Color color(
				255,
				(int)Gosu::random(200.0, 255.0),
				(int)Gosu::random(200.0, 255.0),
				(int)Gosu::random(200.0, 255.0));
		m_AIs[i].walk.setColorMod(color);
		m_AIs[i].run.setColorMod(color);
		m_rendMan.registerRenderable(0, &m_AIs[i].walk);
		m_rendMan.registerRenderable(0, &m_AIs[i].run);
	}
	
	//Pass off song creation/management to manager
	//  this way anyone can pause the music if needed
	m_audMan.createSong(
		Gosu::resourcePrefix() + L"Sound/" + Gosu::widen( jVal.get("Music", "song.mp3").asString()), 
		"Background");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hit1.wav", "hit1");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hit2.wav", "hit2");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hit3.wav", "hit3");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hit4.wav", "hit4");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hurt1.wav", "hurt1");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hurt2.wav", "hurt2");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hurt3.wav", "hurt3");
	m_audMan.createSample(Gosu::resourcePrefix() + L"Sound/hurt4.wav", "hurt4");
	m_audMan.playSong("Background", true);
}

void PlayState::cleanup()
{
	m_Engine->showCursor( false );
}

void PlayState::pause()
{
	
}

void PlayState::resume()
{
	
}

void PlayState::update()
{
	//bool moving = false;
	m_PlayerPos.x += 0.25 + m_Continuous*(0.05);
	m_Run.setSpeed((int)((0.25 + m_Continuous*(0.05))*10));

	//This is a really funny way to do this
	bool hit = false;
	if (m_AnimLock == 16) {
		if (m_Action == 1) {
			m_audMan.playStereoSample("hit1", m_PlayerPos.x, m_PlayerPos.y);
		}
		if (m_Action == 2) {
			m_audMan.playStereoSample("hit2", m_PlayerPos.x, m_PlayerPos.y);
		}
		if (m_Action == 3) {
			m_audMan.playStereoSample("hit3", m_PlayerPos.x, m_PlayerPos.y);
		}
		
		for (int i=0; i<MAX_AI; ++i) {
			if (testPoint(i)) {
				if (m_Action == 1) {
					m_Score += 1 + m_Continuous;
				}
				if (m_Action == 2) {
					m_Score += 1 + 2*m_Continuous;
				}
				if (m_Action == 3) {
					m_Score += 1 + 3*m_Continuous;
				}
				m_AIs[i].speed += 0.05;
				m_AIs[i].walking = false;
				m_AIs[i].run.setVisible(true);
				m_AIs[i].walk.setVisible(false);

				m_AIs[i].walk.setSpeed((int)(m_AIs[i].speed*20));
				m_AIs[i].run.setSpeed((int)(m_AIs[i].speed*20));
				hit = true;
				m_Continuous += 1;

				switch ((int)Gosu::random(1.0, 7.0)) {
				case 1:
					m_audMan.playStereoSample("hurt1", m_AIs[i].pos.x, m_AIs[i].pos.y);
					break;
				case 2:
					m_audMan.playStereoSample("hurt2", m_AIs[i].pos.x, m_AIs[i].pos.y);
					break;
				case 3:
					m_audMan.playStereoSample("hurt3", m_AIs[i].pos.x, m_AIs[i].pos.y);
					break;
				case 4:
					m_audMan.playStereoSample("hurt4", m_AIs[i].pos.x, m_AIs[i].pos.y);
					break;
				}
				
			}
		}
		if (!hit) {
			m_Continuous = 0;
		}
	}
	if (m_AnimLock == 0) {
		m_LPunch.setVisible(false);
		m_RPunch.setVisible(false);
		m_HButt.setVisible(false);
		m_Run.setVisible(true);

		m_Run.setFrame(0);

		m_AnimLock -= 1;
	}else{
		m_AnimLock -= 1;
	}
	InputManager *iman = InputManager::getCurrentContext();
	if (iman->query("Play.PunchLeft") == InputManager::actnBegin) {
		m_Action = 1;

		m_LPunch.setVisible(true);
		m_RPunch.setVisible(false);
		m_HButt.setVisible(false);
		m_Run.setVisible(false);

		m_LPunch.setFrame(0);

		m_AnimLock = 32;
	}
	if (iman->query("Play.PunchRight") == InputManager::actnBegin) {
		m_Action = 2;

		m_LPunch.setVisible(false);
		m_RPunch.setVisible(true);
		m_HButt.setVisible(false);
		m_Run.setVisible(false);
		
		m_RPunch.setFrame(0);

		m_AnimLock = 32;
	}
	if (iman->query("Play.HeadButt") == InputManager::actnBegin) {
		m_Action = 3;

		m_LPunch.setVisible(false);
		m_RPunch.setVisible(false);
		m_HButt.setVisible(true);
		m_Run.setVisible(false);
		
		m_HButt.setFrame(0);

		m_AnimLock = 32;
	}
	
	//Update camera focus
	m_Focus[0] = m_PlayerPos.x;
	m_Focus[1] = m_PlayerPos.y - 24.0;

	m_Idle.setX(m_PlayerPos.x);
	m_Idle.setY(m_PlayerPos.y);
	
	m_Run.setX(m_PlayerPos.x);
	m_Run.setY(m_PlayerPos.y);
	
	m_LPunch.setX(m_PlayerPos.x);
	m_LPunch.setY(m_PlayerPos.y);

	m_RPunch.setX(m_PlayerPos.x);
	m_RPunch.setY(m_PlayerPos.y);
	
	m_HButt.setX(m_PlayerPos.x);
	m_HButt.setY(m_PlayerPos.y);

	for (int i=0; i<MAX_AI; ++i) {
		if (m_AIs[i].walking) 
			m_AIs[i].pos.x += m_AIs[i].speed;
		else
			m_AIs[i].pos.x += m_AIs[i].speed * 2.5;

		m_AIs[i].walk.setX(m_AIs[i].pos.x);
		m_AIs[i].walk.setY(m_AIs[i].pos.y);

		m_AIs[i].run.setX(m_AIs[i].pos.x);
		m_AIs[i].run.setY(m_AIs[i].pos.y);
	}

	m_rendMan.setCamera( m_Focus[0], m_Focus[1], m_Zoom, m_Rot);
	m_rendMan.update();
	m_Graph.update();
}

void PlayState::draw() const
{
	// Canvas color
	m_Engine->graphics().drawQuad( 0, 0, m_canvasColor, 
		m_Width, 0, m_canvasColor,
		0, m_Height, m_canvasColor,
		m_Width, m_Height, m_canvasColor, -10);
	
	// Render all
	//m_rendMan->setColor(m_colorMod);
	m_rendMan.doRender();

	m_hud.draw(60.0, 20.0);
	m_Font->drawRel(boost::lexical_cast<std::wstring>(m_Score), 150, 10, 10.0, 1.0, 0.0, 2.0, 2.0);
	if (m_Continuous > 0) {
		m_hud2.draw(250.0, 20.0);
		m_Font->drawRel(boost::lexical_cast<std::wstring>(m_Continuous), 310, 10, 10.0, 1.0, 0.0, 2.0, 2.0);
	}
}


bool PlayState::testPoint(int i)
{
	b2Transform tx;
	tx.position = m_AIs[i].pos;
	tx.R = b2Mat22(0);
	return m_Shape.TestPoint(tx, b2Vec2(m_PlayerPos.x + 2.0f, m_PlayerPos.y));
}