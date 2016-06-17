/************************************************************************************

Filename    :   OvrApp.cpp
Content     :   Trivial use of the application framework.
Created     :   
Authors     :   

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "OvrApp.h"
#include "GuiSys.h"
#include "OVR_Locale.h"
#include "ReadStlUtil.h"
#include "BitmapFont.h"
#include "Android/JniUtils.h"
#include <OVR_Capture.h>
#include "Kernel/OVR_String_Utils.h"

using namespace OVR;

#if defined( OVR_OS_ANDROID )
extern "C" {

jlong Java_oculus_MainActivity_nativeSetAppInterface( JNIEnv * jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	LOG( "nativeSetAppInterface" );
	return (new OvrTemplateApp::OvrApp())->SetActivity( jni, clazz, activity, fromPackageName, commandString, uriString );
}

void Java_oculus_MainActivity_nativeReciveData( JNIEnv *jni, jclass clazz, jlong interfacePtr , int value )
{
	// This is called by the java UI thread.
	OvrTemplateApp::OvrApp * ovrApp = static_cast< OvrTemplateApp::OvrApp * >( ( ( OVR::App * )interfacePtr )->GetAppInterface() );
	//JavaUTFChars utfstring(jni, receiveString);
	const char * msg = ovrApp->GetMessageQueue().GetNextMessage();
	int iMaxValue = value;
	if ( msg != NULL )
	{
		int iLastValue = 0.0;
		sscanf( msg, "%d", &iLastValue);
		iMaxValue = OVRMath_Max(iLastValue, value);
	}
	free( (void *)msg );
	ovrApp->GetMessageQueue().ClearMessages();
	ovrApp->GetMessageQueue().PostPrintf( "%d", iMaxValue );

}

} // extern "C"

#endif

namespace OvrTemplateApp
{

//==============================================================
// ovrGuiSoundEffectPlayer
	class ovrGuiSoundEffectPlayer : public OvrGuiSys::SoundEffectPlayer
	{
	public:
		ovrGuiSoundEffectPlayer( ovrSoundEffectContext & context )
				: SoundEffectContext( context )
		{
		}

		virtual bool Has( const char * name ) const OVR_OVERRIDE { return SoundEffectContext.GetMapping().HasSound( name ); }
		virtual void Play( const char * name ) OVR_OVERRIDE { SoundEffectContext.Play( name ); }

	private:
		ovrSoundEffectContext & SoundEffectContext;
	};

OvrApp::OvrApp()
	: SoundEffectContext( NULL )
	, SoundEffectPlayer( NULL )
	, GuiSys( OvrGuiSys::Create() )
	, Locale( NULL )
    , m_MessageQueue(100)
,m_pBoxModelFile(NULL)
,m_bFighting(false)
{
	m_pAnimationMgr = new AnimationManager(&m_boxInScene);
}

OvrApp::~OvrApp()
{
	OvrGuiSys::Destroy( GuiSys );
	delete m_pAnimationMgr;
}

void OvrApp::Configure( ovrSettings & settings )
{
	settings.PerformanceParms.CpuLevel = 2;
	settings.PerformanceParms.GpuLevel = 2;
}

void OvrApp::OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI )
{
	OVR_UNUSED( fromPackage );
	OVR_UNUSED( launchIntentJSON );
	OVR_UNUSED( launchIntentURI );

	OVR::Capture::InitForRemoteCapture(OVR::Capture::All_Flags);

	const ovrJava * java = app->GetJava();
	SoundEffectContext = new ovrSoundEffectContext( *java->Env, java->ActivityObject );
	SoundEffectContext->Initialize();
	SoundEffectPlayer = new ovrGuiSoundEffectPlayer(*SoundEffectContext);



	Locale = ovrLocale::Create( *app, "default" );

	String fontName;
	GetLocale().GetString( "@string/font_name", "efigs.fnt", fontName );
	GuiSys->Init( this->app, *SoundEffectPlayer, fontName.ToCStr(), &app->GetDebugLines() );
        
	const OvrStoragePaths & paths = app->GetStoragePaths();

	Array<String> SearchPaths;
	paths.PushBackSearchPathIfValid( EST_SECONDARY_EXTERNAL_STORAGE, EFT_ROOT, "RetailMedia/", SearchPaths );
	paths.PushBackSearchPathIfValid( EST_SECONDARY_EXTERNAL_STORAGE, EFT_ROOT, "", SearchPaths );
	paths.PushBackSearchPathIfValid( EST_PRIMARY_EXTERNAL_STORAGE, EFT_ROOT, "RetailMedia/", SearchPaths );
	paths.PushBackSearchPathIfValid( EST_PRIMARY_EXTERNAL_STORAGE, EFT_ROOT, "", SearchPaths );

	const char * scenePath = "Oculus/tuscany.ovrscene";//home_theater tuscany .ovrscene boxOffice
	String SceneFile;
	if ( GetFullPath( SearchPaths, scenePath, SceneFile ) )
	{
		MaterialParms materialParms;
		materialParms.UseSrgbTextureFormats = false;
		Scene.LoadWorldModel( SceneFile.ToCStr(), materialParms );
		//Scene.SetYawOffset( -Mathf::Pi * 0.5f );
	}
	else
	{
		LOG( "OvrApp::OneTimeInit SearchPaths failed to find %s", scenePath );
	}


	// text add to scene
	m_textInScene.SetModelFile(&m_textModelFile);
	Scene.AddModel(&m_textInScene);
	// set text modelMatrix

	//m_textInScene.State.DontRenderForClientUid = 1;	// default is -1 not render scene#frame

	// text
	String s = String::Format("Hit it!");
	float textScale = 2.0;
	Vector4f textColor = Vector4f(0.1, 0.1, 1.0, 1.0);
	m_OvrSurfaceTextDef.geo.Free();
	m_OvrSurfaceTextDef = app->GetDebugFont().TextSurface(s.ToCStr(), textScale, textColor, HORIZONTAL_LEFT, VERTICAL_BASELINE);

	m_textModelFile.Def.surfaces.Clear();
	m_textModelFile.Def.surfaces.PushBack(m_OvrSurfaceTextDef);

	// stl

	m_CubeProgram = BuildProgram(
			VertexColorVertexShaderSrc
			,
			VertexColorFragmentShaderSrc
	);
	m_OvrSurfaceDef.materialDef.programObject = m_CubeProgram.program;
	m_OvrSurfaceDef.materialDef.uniformMvp = m_CubeProgram.uMvp;

	const char * stlPath = "Oculus/arm_base.stl";//"Oculus/boxing_gloves.FBX";
	String stlFile;
	if ( GetFullPath( SearchPaths, stlPath, stlFile ) )
	{
		VertexAttribs attribs;
		Array< TriangleIndex > indices;
		CReadStlUtil::ReadStlNode(stlFile.ToCStr(), attribs, indices, m_OvrSurfaceDef.cullingBounds);
		m_OvrSurfaceDef.geo.Create( attribs, indices );

		m_stlModelFile.Def.surfaces.Clear();
		m_stlModelFile.Def.surfaces.PushBack(m_OvrSurfaceDef);

		//stl
		m_stlModelInScene.SetModelFile(&m_stlModelFile);
		//Scene.AddModel(&m_stlModelInScene);
		//m_stlModelInScene.State.DontRenderForClientUid = 1;

//		ovrDrawSurface drawSruface2;
//		drawSruface2.modelMatrix = &mModelMatrix;
//		drawSruface2.joints = NULL;
//		drawSruface2.surface = &mOvrSurfaceDef;
//		Scene.GetEmitList().PushBack(drawSruface2);
	}

	// box model file
	const char * boxPath = "Oculus/boxing_gloves2.ovrscene";
	String boxFile;
	if (GetFullPath(SearchPaths, boxPath, boxFile))
	{

		MaterialParms materialParms;
		m_pBoxModelFile = LoadModelFile(boxFile.ToCStr(), Scene.GetDefaultGLPrograms(), materialParms);
		if (m_pBoxModelFile != NULL)
		{
			m_boxInScene.SetModelFile(m_pBoxModelFile);
			Scene.AddModel(&m_boxInScene);

			Posef boxPos;
			boxPos.Position = Vector3f(0.0f, 0.0f, -2.0f);
			Matrix4f boxMat(boxPos);
			boxMat *= Matrix4f::Scaling(0.001);
			m_boxInScene.State.modelMatrix = boxMat;
			m_boxInScene.State.DontRenderForClientUid = -1;// defaule not render it

		}
	}

	m_dOneTimeInit = vrapi_GetTimeInSeconds();
	//Scene.SetFreeMove(true);


}

void OvrApp::OneTimeShutdown()
{

	OVR::Capture::Log(OVR::Capture::Log_Info, "OneTimeShutdown");
	OVR::Capture::Shutdown();

	delete SoundEffectPlayer;
	SoundEffectPlayer = NULL;

	delete SoundEffectContext;
	SoundEffectContext = NULL;



	m_OvrSurfaceTextDef.geo.Free();

	m_OvrSurfaceDef.geo.Free();
	DeleteProgram( m_CubeProgram );
	if (m_pBoxModelFile != NULL)
	{
		delete m_pBoxModelFile;
		m_pBoxModelFile = NULL;
	}
}

bool OvrApp::OnKeyEvent( const int keyCode, const int repeatCount, const KeyEventType eventType )
{
	if ( GuiSys->OnKeyEvent( keyCode, repeatCount, eventType ) )
	{
		return true;
	}
	return false;
}

Matrix4f OvrApp::Frame( const VrFrame & vrFrame )
{
	HandleMessage();
	Update(vrFrame);
	// Player movement.
	Scene.Frame( vrFrame, app->GetHeadModelParms(), -1);

	// Update GUI systems after the app frame, but before rendering anything.
	GuiSys->Frame( vrFrame, Scene.GetCenterEyeViewMatrix() );

	return Scene.GetCenterEyeViewMatrix();
}

Matrix4f OvrApp::DrawEyeView( const int eye, const float fovDegreesX, const float fovDegreesY, ovrFrameParms & frameParms )
{
	const Matrix4f viewMatrix = Scene.GetEyeViewMatrix( eye );
	const Matrix4f projectionMatrix = Scene.GetEyeProjectionMatrix( eye, fovDegreesX, fovDegreesY );

	// back color
	glClearColor( 0.125f, 0.0f, 0.125f, 1.0f ) ;
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

	const Matrix4f eyeViewProjection = Scene.DrawEyeView( eye, fovDegreesX, fovDegreesY );

	frameParms.ExternalVelocity = Scene.GetExternalVelocity();
	frameParms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

	GuiSys->RenderEyeView( Scene.GetCenterEyeViewMatrix(), viewMatrix, projectionMatrix );

	return eyeViewProjection;
}

	void OvrApp::Command( const char * msg )
	{
		int iValue = 0.0;
		sscanf( msg, "%d", &iValue);
		m_iMaxValue = OVRMath_Max(m_iMaxValue, iValue);
//		String s = String::Format("%s", msg);
//		float textScale = 1.0;
//		Vector4f textColor = Vector4f(1.0, 0.0, 0.0, 1.0);
//		m_OvrSurfaceTextDef.geo.Free();
//		m_OvrSurfaceTextDef = app->GetDebugFont().TextSurface(s.ToCStr(), textScale, textColor, HORIZONTAL_LEFT, VERTICAL_BASELINE);
//
//		m_textModelFile.Def.surfaces.Clear();
//		m_textModelFile.Def.surfaces.PushBack(m_OvrSurfaceTextDef);
	}

	void OvrApp::GetVoiceNameAndSpeed(int hitValue, String &strVoice, double &dSpeed, OVR::String &strMessage) const
	{
		String message[8] ={"are you ok", "go home", "oh no", "yes", "good",
		"very good", "oh my god", "you are monster"};
		int iIndex = hitValue / 80;
		if (iIndex < 0)
			iIndex = 0;
		if (iIndex > 7)
			iIndex = 7;

		dSpeed = 0.6 + 0.125 * iIndex;
		strVoice = String::Format("%d", iIndex + 1);
		strMessage = message[iIndex];

	}
	void OvrApp::HandleMessage()
	{
		static bool bFirst = true;
		double dTimeNow = vrapi_GetTimeInSeconds();
		if (bFirst)
		{
			if (dTimeNow - m_dOneTimeInit > 15.0)
			{
				if (m_pBoxModelFile != NULL)
				{
					OVR::Capture::Log(OVR::Capture::Log_Info, "is not null");
					int iSize = m_pBoxModelFile->Def.surfaces.GetSizeI();
					OVR::Capture::Logf(OVR::Capture::Log_Info, "surface size: %d", iSize);
					for (int i = 0; i < iSize; ++i) {
						const ovrSurfaceDef &surfacceDef = m_pBoxModelFile->Def.surfaces[i];
						OVR::Capture::Logf(OVR::Capture::Log_Info, "surface0: %d boundry: %s", i, StringUtils::ToString(surfacceDef.cullingBounds).ToCStr());
						OVR::Capture::Logf(OVR::Capture::Log_Info, "surface0: %d vertexCount: %d", i,  surfacceDef.geo.vertexCount);
						OVR::Capture::Logf(OVR::Capture::Log_Info, "surface0: %d indexCount: %d", i,  surfacceDef.geo.indexCount);
					}

					OVR::Capture::Log(OVR::Capture::Log_Info, "stl info");
					OVR::Capture::Logf(OVR::Capture::Log_Info, "stl: %d boundry: %s", 1, StringUtils::ToString(m_OvrSurfaceDef.cullingBounds).ToCStr());
					OVR::Capture::Logf(OVR::Capture::Log_Info, "stl: %d vertexCount: %d", 1,  m_OvrSurfaceDef.geo.vertexCount);
					OVR::Capture::Logf(OVR::Capture::Log_Info, "stl: %d indexCount: %d", 1,  m_OvrSurfaceDef.geo.indexCount);
				}
				else
				{
					OVR::Capture::Log(OVR::Capture::Log_Info, "is null");
				}
				bFirst = false;
			}
		}
		for ( ; ; )
		{
			const char * msg = m_MessageQueue.GetNextMessage();
			if ( msg == NULL )
			{
				break;
			}
			if (!m_bFighting)
			{
				m_bFighting = true;
				// begin fight
				m_iMaxValue = 0;
			}
			Command( msg );
			free( (void *)msg );
			m_dFightTime = dTimeNow;
		}
		if(m_bFighting && (dTimeNow - m_dFightTime) > 1.0)
		{
			m_bFighting = false;
			// begin animation
			if (!m_pAnimationMgr->IsAnimationing())
			{
				String strVoice;
				double dSpeed = 0.5;
				GetVoiceNameAndSpeed(m_iMaxValue, strVoice, dSpeed, m_strMessage);
				SoundEffectPlayer->Play(strVoice.ToCStr());
				m_pAnimationMgr->BeginAnimation(dTimeNow, dSpeed, Scene.GetEyeYaw());
//				String s = "come on!";
//				float textScale = 2.0;
//				Vector4f textColor = Vector4f(0.1, 0.1, 1.0, 1.0);
//				m_OvrSurfaceTextDef.geo.Free();
//				m_OvrSurfaceTextDef = app->GetDebugFont().TextSurface(s.ToCStr(), textScale, textColor, HORIZONTAL_LEFT, VERTICAL_BASELINE);
//
//				m_textModelFile.Def.surfaces.Clear();
//				m_textModelFile.Def.surfaces.PushBack(m_OvrSurfaceTextDef);
			}
		}
	}
	void OvrApp::Update(const OVR::VrFrame &vrFrame)
	{
		if (m_pAnimationMgr->Update(vrFrame))
		{
			// end animation
			String s = String::Format("%s %d", m_strMessage.ToCStr(), m_iMaxValue);
			float textScale = 2.0;
			Vector4f textColor = Vector4f(0.1, 0.1, 1.0, 1.0);
			m_OvrSurfaceTextDef.geo.Free();
			m_OvrSurfaceTextDef = app->GetDebugFont().TextSurface(s.ToCStr(), textScale, textColor, HORIZONTAL_LEFT, VERTICAL_BASELINE);

			m_textModelFile.Def.surfaces.Clear();
			m_textModelFile.Def.surfaces.PushBack(m_OvrSurfaceTextDef);
		}

		// update text pos
		Vector3f textPt = Matrix4f::RotationY(Scene.GetEyeYaw()).Transform(Vector3f(-0.5f, 2.0f, -1.5f));
		Posef textPos;
		textPos.Position = textPt;
		Matrix4f textMat(textPos);
		textMat *= Matrix4f::RotationY(Scene.GetEyeYaw());
		m_textInScene.State.modelMatrix = textMat;
		//float f1 = vrFrame.Input.sticks[0][0];
		//float f2 = vrFrame.Input.sticks[0][1];
		//OVR::Capture::Logf(OVR::Capture::Log_Info, "input1: %f input2: %f", f1,  f2);

		//text update

		// todo move to frame

		// stl model pos
//		static float mfx = 0.0f;
//		float fsx = -1.0f, fex = 1.0f;
//		static float fStep = 0.01f;
//		mfx += fStep;
//		if (mfx > fex)
//		{
//			mfx = fsx;
//			//sv_panel_touch_up
//			//SoundEffectPlayer->Play("can");
//		}
//
////		static float ss1 = ( rand() & 65535 ) / (65535.0f / 2.0f) - 1.0f;
////		static float ss2 = ( rand() & 65535 ) / (65535.0f / 2.0f) - 1.0f;
////		static float ss3 = ( rand() & 65535 ) / (65535.0f / 2.0f) - 1.0f;
////		float fTmie = vrapi_GetTimeInSeconds();
////		const ovrMatrix4f rotation = ovrMatrix4f_CreateRotation(
////				ss1 *fTmie,
////				ss2 *fTmie,
////				ss3 *fTmie);
////		const ovrMatrix4f translation = ovrMatrix4f_CreateTranslation(
////				mfx,
////				1.2,
////				-2.0f );
////		const ovrMatrix4f matTT = ovrMatrix4f_Multiply( &translation, &rotation );
//		Posef stlPos;
//		stlPos.Position = Vector3f(mfx, 1.2f, -2.0f);
//		//stlPos.Orientation = Quatf(ss1 *fTmie, ss2 * fTmie, ss3 * fTmie, 2.0f);
//		//stlPos.Rotate(Vector3f(ss1 *fTmie, ss2 * fTmie, ss3 * fTmie));
//		Matrix4f stlMatrix(stlPos);
//
//		m_stlModelInScene.State.modelMatrix = stlMatrix;

		// box move form z 0 to front
//		static float boxZ = 0.0f;
//		float fBoxS = 0.1f;
//		float fBoxE = -1.0f;
//		boxZ -= fStep;
//		if (boxZ < fBoxE)
//		{
//			boxZ = fBoxS;
//			SoundEffectPlayer->Play("can");
//		}
//		Posef boxPos;
//		boxPos.Position = Vector3f(0.4f, 1.5f, boxZ);
//		//boxPos.Orientation = Quatf(Matrix4f::RotationZ(-Mathf::PiOver2));
//		Matrix4f boxMatrix(boxPos);
//
//		boxMatrix *= Matrix4f::Scaling(0.001);
//		boxMatrix *= Matrix4f::RotationY(Mathf::PiOver2);
//		boxMatrix *= Matrix4f::RotationZ(-Mathf::PiOver2);
//		m_boxInScene.State.modelMatrix = boxMatrix;
	}

} // namespace OvrTemplateApp
