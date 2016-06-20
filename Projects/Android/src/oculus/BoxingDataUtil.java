package oculus;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

/**
 * Created by jun on 2016/6/20.
 */
public class BoxingDataUtil {
    private final String TAG = "BoxingDataUtil";

    private byte[] mReceiveDatas;

    private final Handler mHandler;

    private boolean mHiting;

    private int mMaxHitValue;

    private final int mMinValidValue = 0;

    public BoxingDataUtil( Handler handler)
    {
        mReceiveDatas = null;
        mHandler = handler;
        mHiting = false;
    }

    private int Get16BitValue(byte[] datas, int iStartIndex)
    {
        int value= 0;
        //
        for (int i = 0; i < 2; i++) {
            int shift=  i * 8;
            value +=(mReceiveDatas[i+ iStartIndex] & 0x000000FF) << shift;//
        }
        return value;
    }

    public synchronized void AddNewData(byte[] readBuf, int count)
    {
        mReceiveDatas = ByteUtil.Merge(mReceiveDatas, readBuf, count);
        if(null == mReceiveDatas)
            return;

        // find rangeData
        final byte byteStart = (byte)0xAC;
        final byte byteEnd = (byte)0xEF;
        int iByteEndIndex = -1;
        int iLastByteStartIndex = -1;
        while (true)
        {
            boolean bFind = false;
            int iByteStartIndex = -1;
            for(int iIndex = iByteEndIndex+1; iIndex < mReceiveDatas.length;iIndex++)
            {
                if(iByteStartIndex < 0)// not find start
                {
                    if(byteStart == mReceiveDatas[iIndex])// find start
                    {
                        iByteStartIndex = iIndex;
                        iLastByteStartIndex = iIndex;
                    }
                }
                else if(byteEnd == mReceiveDatas[iIndex])//find end
                {
                    iByteEndIndex = iIndex;
                    iLastByteStartIndex = -1;
                    bFind = true;
                    break;
                }
            }
            if(!bFind)
            {
                break;
            }
            //
            if (iByteEndIndex - iByteStartIndex == 9)// 10 bytes
            {
                //int iLength = mReceiveDatas[iByteStartIndex+1];
                //Log.i(TAG,"iLength:"+Integer.toString(iLength));
                int iType = mReceiveDatas[iByteStartIndex+2];
                //Log.i(TAG, "iType:"+Integer.toString(iType));
                //int iLR = mReceiveDatas[iByteStartIndex+3];
                //Log.i(TAG, "LR:"+Integer.toString(iLR));
                if(19 == iType)
                {
                    int value = Get16BitValue(mReceiveDatas, iByteStartIndex+4);
                    if (value == 0) {
                        Log.i(TAG, "iValue:" + Integer.toString(value));
                    }
                    // imu data
                    int imux = Get16BitValue(mReceiveDatas,iByteStartIndex + 5);
                    int imuy = Get16BitValue(mReceiveDatas, iByteStartIndex+6);
                    int imuz = Get16BitValue(mReceiveDatas, iByteStartIndex+7);
                    Log.i(TAG, "imux:" + Integer.toString(imux));
                    Log.i(TAG, "imuy:" + Integer.toString(imuy));
                    Log.i(TAG, "imuz:" + Integer.toString(imuz));

                    AddNewData(ConvertValue(value));
//                    if (value < 650) {
//                        nativeReciveData(getAppPtr(), 650 - value);
//                    }
                }
            }
        }
        int iRemoveCount = 0;
        if (-1 == iLastByteStartIndex)
        {
            iRemoveCount = mReceiveDatas.length;
        }
        else
        {
            iRemoveCount = iLastByteStartIndex;
        }
        mReceiveDatas = ByteUtil.GetLastBytes(mReceiveDatas, iRemoveCount);
    }

    private int ConvertValue(int value) {
        return 650 - value;
    }

    private void AddNewData(int value) {
        if(!IsValidData(value))
        {
            return;
        }
        else
        {
            if(mHiting)
            {
                mHiting = false;
                EndHiting();
            }
        }
        if(!mHiting)
        {
            mMaxHitValue = mMinValidValue;
            mHiting = true;
            BeginHiting();
        }
        mMaxHitValue = Math.max(mMaxHitValue, value);
    }

    private void EndHiting() {
        Message msg = mHandler.obtainMessage(Constants.MESSAGE_DEVICE_NAME);
        Bundle bundle = new Bundle();
        bundle.putInt(Constants.HitMaxValue, mMaxHitValue);
        msg.setData(bundle);
        mHandler.sendMessage(msg);
    }

    private void BeginHiting() {

    }

    private boolean IsValidData(int value) {
        if(value >= mMinValidValue)
            return true;
        return  false;
    }

}
