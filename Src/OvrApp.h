/************************************************************************************

Filename    :   OvrApp.h
Content     :   Trivial use of the application framework.
Created     :   
Authors     :   

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVRAPP_H
#define OVRAPP_H

#include "App.h"
#include "SceneView.h"
#include "SoundEffectContext.h"
#include <memory>
#include "GuiSys.h"

namespace OvrTemplateApp
{

class OvrApp : public OVR::VrAppInterface
{
public:
						OvrApp();
	virtual				~OvrApp();

	virtual void 		Configure( OVR::ovrSettings & settings )OVR_OVERRIDE;
	virtual void		OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI )OVR_OVERRIDE;
	virtual void		OneTimeShutdown()OVR_OVERRIDE;
	virtual bool 		OnKeyEvent( const int keyCode, const int repeatCount, const OVR::KeyEventType eventType )OVR_OVERRIDE;
	virtual OVR::Matrix4f Frame( const OVR::VrFrame & vrFrame )OVR_OVERRIDE;
	virtual OVR::Matrix4f DrawEyeView( const int eye, const float fovDegreesX, const float fovDegreesY, ovrFrameParms & frameParms )OVR_OVERRIDE;

	class OVR::ovrLocale &	GetLocale() { return *Locale; }

	OVR::ovrMessageQueue &	GetMessageQueue() { return m_MessageQueue; }

	// update before frame
	void Update(const OVR::VrFrame &vrFrame);

private:
	void 				HandleMessage();
	void				Command( const char * msg );

	OVR::ovrSoundEffectContext * SoundEffectContext;
	OVR::OvrGuiSys::SoundEffectPlayer * SoundEffectPlayer;
	OVR::OvrGuiSys *		GuiSys;
	class OVR::ovrLocale *	Locale;

	OVR::OvrSceneView		Scene;

	double m_dOneTimeInit;

	//message
	OVR::ovrMessageQueue		m_MessageQueue;


	// text
	OVR::ovrSurfaceDef m_OvrSurfaceTextDef;
	OVR::ModelInScene m_textInScene;
	OVR::ModelFile m_textModelFile;

	// stl model
	OVR::GlProgram m_CubeProgram;
	OVR::ovrSurfaceDef m_OvrSurfaceDef;
	OVR::ModelInScene m_stlModelInScene;
	OVR::ModelFile m_stlModelFile;

	// box model
	OVR::ModelInScene m_boxInScene;
	OVR::ModelFile *m_pBoxModelFile;

};

} // namespace OvrTemplateApp

#endif	// OVRAPP_H
