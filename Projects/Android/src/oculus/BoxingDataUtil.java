package oculus;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;


import java.lang.Integer;
import java.lang.Long;
import java.lang.System;

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

    private long mHitTime;

    private double mdSpeedValue;

    private final int mSpeedQueueCount = 10;

    private int []mSpeedQueue = new int[mSpeedQueueCount];
    int mIndex = 0;

    private void AddToSpeedQueue(int iValue)
    {
        mSpeedQueue[mIndex] = iValue;
        mIndex++;
        mIndex %=mSpeedQueueCount;
    }

    private double GetSpeedValue()
    {
        int iSpeedCount = 0;
        int iAll = 0;
        for(int i = 0; i < mSpeedQueueCount; i++)
        {
            if (mSpeedQueue[i] < -2300)
            {
                iSpeedCount++;
                iAll+=-mSpeedQueue[i];
            }
        }
        if(iSpeedCount > 0)
            return (double)iAll / (double)iSpeedCount;
        return 0.0;
    }


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
        if(null == mReceiveDatas) {
            return;
        }

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
            if (iByteEndIndex - iByteStartIndex == 10)// 11 bytes
            {
                //int iLR = mReceiveDatas[iByteStartIndex+1];
                //Log.i(TAG, "LR:"+Integer.toString(iLR));

                int value = Get16BitValue(mReceiveDatas, iByteStartIndex+2);
                if (value == 0) {
                    Log.i(TAG, "iValue:" + Integer.toString(value));
                }
                Log.i(TAG, "iValue:" + Integer.toString(value));
                // imu data
                //float imux = (float)Get16BitValue(mReceiveDatas,iByteStartIndex + 4) / 100.0f;
                int imuy =  Get16BitValue(mReceiveDatas, iByteStartIndex + 6);
                //float imuz =  (float)Get16BitValue(mReceiveDatas, iByteStartIndex+8) / 100.0f;

                //double dTemp = imux * imux+imuy*imuy+imuz*imuz;;

                //double dValue = Math.sqrt(dTemp);
//                Log.i(TAG, "imux:" + Float.toString(imux));
                Log.i(TAG, "imuy:" + Float.toString(imuy));
//                Log.i(TAG, "imuz:" + Float.toString(imuz));
//                Log.i(TAG, "imuHe:" + Double.toString(dTemp));
                //Log.i(TAG, "imu:" + Double.toString(dValue));

                AddNewData(ConvertValue(value), imuy);

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

    private void AddNewData(int value, int iSpeedValue) {
        long lTimeNow = System.currentTimeMillis();
        //mdMaxSpeedValue = Math.max(mdMaxSpeedValue, dSpeedValue);
        AddToSpeedQueue(iSpeedValue);
        if(IsValidData(value))
        {
            if(!mHiting)
            {
                BeginHiting();
            }
            mMaxHitValue = Math.max(mMaxHitValue, value);
            mHitTime = lTimeNow;
        }
        else
        {
            if(mHiting && (lTimeNow - mHitTime) > 500)
            {
                // if 500 millisecond no hit ,then hit end
                EndHiting();
            }
        }
    }

    private void EndHiting() {
        mHiting = false;
        // hand message
        Message msg = mHandler.obtainMessage(Constants.MESSAGE_BOXING_END);
        Bundle bundle = new Bundle();
        bundle.putInt(Constants.HitMaxValue, mMaxHitValue);
        bundle.putDouble(Constants.MoveSpeed, mdSpeedValue);
        msg.setData(bundle);
        mHandler.sendMessage(msg);
    }

    private void BeginHiting() {
        mHiting = true;
        mMaxHitValue = mMinValidValue;
        mdSpeedValue = GetSpeedValue();
    }

    private boolean IsValidData(int value) {
        if(value >= mMinValidValue)
            return true;
        return  false;
    }

}
