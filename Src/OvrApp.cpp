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

using namespace OVR;

#if defined( OVR_OS_ANDROID )
extern "C" {

jlong Java_oculus_MainActivity_nativeSetAppInterface( JNIEnv * jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	LOG( "nativeSetAppInterface" );
	return (new OvrTemplateApp::OvrApp())->SetActivity( jni, clazz, activity, fromPackageName, commandString, uriString );
}

void Java_oculus_MainActivity_nativeReciveData( JNIEnv *jni, jlong interfacePtr , jstring receiveString )
{
	// This is called by the java UI thread.
	OvrTemplateApp::OvrApp * ovrApp = static_cast< OvrTemplateApp::OvrApp * >( ( ( OVR::App * )interfacePtr )->GetAppInterface() );
	JavaUTFChars utfstring(jni, receiveString);
	ovrApp->GetMessageQueue().ClearMessages();
	ovrApp->GetMessageQueue().PostPrintf( "%s", utfstring.ToStr() );

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
{
}

OvrApp::~OvrApp()
{
	OvrGuiSys::Destroy( GuiSys );
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

	const char * scenePath = "Oculus/tuscany.ovrscene";
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
	Posef textPos;
	textPos.Position = Vector3f(0.0f, 2.0f, -2.0f);
	Matrix4f textMat(textPos);
	m_textInScene.State.modelMatrix = textMat;
	m_textInScene.State.DontRenderForClientUid = 1;	// default is -1 not render scene#frame

	// text
	String s = String::Format("no message recive!");
	float textScale = 1.0;
	Vector4f textColor = Vector4f(1.0, 0.0, 0.0, 1.0);
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
		Scene.AddModel(&m_stlModelInScene);
		m_stlModelInScene.State.DontRenderForClientUid = 1;

//		ovrDrawSurface drawSruface2;
//		drawSruface2.modelMatrix = &mModelMatrix;
//		drawSruface2.joints = NULL;
//		drawSruface2.surface = &mOvrSurfaceDef;
//		Scene.GetEmitList().PushBack(drawSruface2);
	}
	m_dOneTimeInit = vrapi_GetTimeInSeconds();
}

void OvrApp::OneTimeShutdown()
{
	delete SoundEffectPlayer;
	SoundEffectPlayer = NULL;

	delete SoundEffectContext;
	SoundEffectContext = NULL;



	m_OvrSurfaceTextDef.geo.Free();

	m_OvrSurfaceDef.geo.Free();
	DeleteProgram( m_CubeProgram );
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
		String s = String::Format("%s", msg);
		float textScale = 1.0;
		Vector4f textColor = Vector4f(1.0, 0.0, 0.0, 1.0);
		m_OvrSurfaceTextDef.geo.Free();
		m_OvrSurfaceTextDef = app->GetDebugFont().TextSurface(s.ToCStr(), textScale, textColor, HORIZONTAL_LEFT, VERTICAL_BASELINE);

		m_textModelFile.Def.surfaces.Clear();
		m_textModelFile.Def.surfaces.PushBack(m_OvrSurfaceTextDef);
	}
	void OvrApp::HandleMessage()
	{
		static bool bFirst = true;
		if (bFirst)
		{
			double dTimeNow = vrapi_GetTimeInSeconds();
			if (dTimeNow - m_dOneTimeInit > 10.0)
			{
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
			Command( msg );
			free( (void *)msg );
		}
	}
	void OvrApp::Update(const OVR::VrFrame &vrFrame)
	{

		//text update

		// todo move to frame

		// stl model pos
		static float mfx = 0.0f;
		float fsx = -1.0f, fex = 1.0f;
		static float fStep = 0.01f;
		mfx += fStep;
		if (mfx > fex)
		{
			mfx = fsx;
			//sv_panel_touch_up
			SoundEffectPlayer->Play("can");
		}

//		static float ss1 = ( rand() & 65535 ) / (65535.0f / 2.0f) - 1.0f;
//		static float ss2 = ( rand() & 65535 ) / (65535.0f / 2.0f) - 1.0f;
//		static float ss3 = ( rand() & 65535 ) / (65535.0f / 2.0f) - 1.0f;
//		float fTmie = vrapi_GetTimeInSeconds();
//		const ovrMatrix4f rotation = ovrMatrix4f_CreateRotation(
//				ss1 *fTmie,
//				ss2 *fTmie,
//				ss3 *fTmie);
//		const ovrMatrix4f translation = ovrMatrix4f_CreateTranslation(
//				mfx,
//				1.2,
//				-2.0f );
//		const ovrMatrix4f matTT = ovrMatrix4f_Multiply( &translation, &rotation );
		Posef stlPos;
		stlPos.Position = Vector3f(mfx, 1.2f, -2.0f);
		//stlPos.Orientation = Quatf(ss1 *fTmie, ss2 * fTmie, ss3 * fTmie, 2.0f);
		//stlPos.Rotate(Vector3f(ss1 *fTmie, ss2 * fTmie, ss3 * fTmie));
		Matrix4f stlMatrix(stlPos);

		m_stlModelInScene.State.modelMatrix = stlMatrix;
	}

} // namespace OvrTemplateApp
