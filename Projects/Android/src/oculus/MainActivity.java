/************************************************************************************

Filename    :   MainActivity.java
Content     :   
Created     :   
Authors     :   

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/
package oculus;


import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.content.Intent;
import com.oculus.vrappframework.VrActivity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;

import java.lang.Override;

public class MainActivity extends VrActivity {
	public static final String TAG = "vrboxing";


	private BluetoothAdapter mBluetoothAdapter;
	private final static String mLDeviceAddress = "20:16:05:05:47:09";
	private final static String mRDeviceAddress = "20:16:05:05:54:93";//"20:16:05:05:47:09";
	private BluetoothChatService mChatService = null;

	private BackgroundMusic mBackgroundMusic = null;

	private byte[] mReceiveDatas = null;
	private BoxingDataUtil mBoxingDataUtil = null;

	boolean m_bFirst = true;

	/** Load jni .so on initialization */
	static {
		Log.d(TAG, "LoadLibrary");
		System.loadLibrary("ovrapp");
	}


    public static native long nativeSetAppInterface( VrActivity act, String fromPackageNameString, String commandString, String uriString );
	public static native void nativeReciveData(long appPtr, int value, double dSpeed);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		Intent intent = getIntent();
		String commandString = VrActivity.getCommandStringFromIntent( intent );
		String fromPackageNameString = VrActivity.getPackageStringFromIntent( intent );
		String uriString = VrActivity.getUriStringFromIntent( intent );

		setAppPtr( nativeSetAppInterface( this, fromPackageNameString, commandString, uriString ) );

		// bluetooth
		mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

		mChatService = new BluetoothChatService( mHandler);

		mBackgroundMusic = BackgroundMusic.getInstance(this);
		mBackgroundMusic.playBackgroundMusic("sql2.mp3", true);

		mBoxingDataUtil = new BoxingDataUtil(mBoxingDataHandler);

    }

	// try to connect device
	// because sometime it wile lost connect
	private void DelyConnectDevice(long delayMillis)
	{
		new Handler().postDelayed(new Runnable(){
			public void run() {
				Log.i(TAG, "delyConnectDevice");
				BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(mRDeviceAddress);
				if (null != device)
				{
					Log.i(TAG, "L device connect");
					mChatService.connect(device, true);
				}
				else
				{
					Log.i(TAG, "L device null");
				}
			}
		}, delayMillis);
	}

	@Override
	public void onResume() {
		super.onResume();

		// Performing this check in onResume() covers the case in which BT was
		// not enabled during onStart(), so we were paused to enable it...
		// onResume() will be called when ACTION_REQUEST_ENABLE activity returns.

		Log.i(TAG, "onResume");
		if (mChatService != null) {
			Log.i(TAG, "mChatService != null");
			// Only if the state is STATE_NONE, do we know that we haven't started already
			if (mChatService.getState() == BluetoothChatService.STATE_NONE) {
				// Start the Bluetooth chat services
				Log.i(TAG, "mChatService start");
				mChatService.start();
				Log.i(TAG, "getRemoteDevice");

				DelyConnectDevice(5000);

//				if(mBluetoothAdapter.getState() != mBluetoothAdapter.STATE_CONNECTING)
//				{
//					//try to connect r device
//					device = mBluetoothAdapter.getRemoteDevice(mRDeviceAddress);
//					if(null != device)
//					{
//						Log.i(TAG, "R device connect");
//						mChatService.connect(device, true);
//					}
//					else
//					{
//						Log.i(TAG, "R device null");
//					}
//				}

			}

		}

		mBackgroundMusic.resumeBackgroundMusic();

	}

	@Override
	protected void onPause()
	{
		super.onPause();
		mBackgroundMusic.pauseBackgroundMusic();
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();

		if (mChatService != null) {
			mChatService.stop();
		}
	}

	protected void AddReceiveData(byte[] readBuf, int count) {
		mBoxingDataUtil.AddNewData(readBuf, count);
	}

	/**
	 * The Handler that gets information back from the BluetoothChatService
	 */
	private final Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			//FragmentActivity activity = getActivity();
			switch (msg.what) {
				case Constants.MESSAGE_STATE_CHANGE:
					switch (msg.arg1) {
						case BluetoothChatService.STATE_CONNECTED:
							//setStatus(getString(R.string.title_connected_to, mConnectedDeviceName));
							//mConversationArrayAdapter.clear();
							break;
						case BluetoothChatService.STATE_CONNECTING:
							//setStatus(R.string.title_connecting);
							break;
						case BluetoothChatService.STATE_LISTEN:
						case BluetoothChatService.STATE_NONE:
							//setStatus(R.string.title_not_connected);
							break;
					}
					break;
				case Constants.MESSAGE_WRITE:
					byte[] writeBuf = (byte[]) msg.obj;
					// construct a string from the buffer
					String writeMessage = new String(writeBuf);
					//mConversationArrayAdapter.add("Me:  " + writeMessage);
					break;
				case Constants.MESSAGE_READ:
					byte[] readBuf = (byte[]) msg.obj;
					AddReceiveData(readBuf, msg.arg1);
					// construct a string from the valid bytes in the buffer
					String readMessage = new String(readBuf, 0, msg.arg1);
					if (m_bFirst) {
						m_bFirst = false;
						Log.i(TAG, readMessage);
					}
					//mTextView.clearComposingText();
					//mTextView.append(readMessage + "\n\n");
					//mConversationArrayAdapter.add(mConnectedDeviceName + ":  " + readMessage);
					break;
				case Constants.MESSAGE_DEVICE_NAME:
					// save the connected device's name
					//mConnectedDeviceName = msg.getData().getString(Constants.DEVICE_NAME);
					//if (null != activity) {
					//   Toast.makeText(activity, "Connected to "
					//           + mConnectedDeviceName, Toast.LENGTH_SHORT).show();
					//}
					Log.i(TAG, "Connected to " + msg.getData().getString(Constants.DEVICE_NAME));
					break;
				case Constants.MESSAGE_TOAST:
					// if (null != activity) {
					//     Toast.makeText(activity, msg.getData().getString(Constants.TOAST),
					//             Toast.LENGTH_SHORT).show();
					//}
					// connect after 5seconds
					DelyConnectDevice(5000);
					Log.i(TAG, "message_toast " + msg.getData().getString(Constants.TOAST));
					break;
			}
		}
	};

	/**
	 * The Handler that gets information back from the BoxingDataUtil
	 */
	private final Handler mBoxingDataHandler = new Handler(){
		@Override
		public void handleMessage(Message msg)
		{
			Log.i(TAG, " mBoxingDataHandler handleMessage ");
			switch (msg.what)
			{
				case Constants.MESSAGE_BOXING_START:
				{
					break;
				}
				case Constants.MESSAGE_BOXING_END:
				{
					int hitValue = msg.getData().getInt(Constants.HitMaxValue);
					double dSpeed = msg.getData().getDouble(Constants.MoveSpeed);
					Log.i(TAG, "boing hit value:"+Integer.toString(hitValue));
					Log.i(TAG, "boing speed value:"+Double.toString(dSpeed));
					nativeReciveData(getAppPtr(), hitValue, dSpeed);
					break;
				}
			}
		}
	};
}
