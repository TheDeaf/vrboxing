package oculus;

/**
 * Created by jun on 2016/6/15.
 */
public class ByteUtil {
    public static int GetSize(byte[] value)
    {
        if (null == value)
            return 0;
        return value.length;
    }
    public static byte[] Merge(byte[] first, byte[] second, int secondcount) {
        return Merge(first, GetFirstBytes(second, secondcount));
    }
    public static byte[] Merge(byte[] first, byte[] second)
    {
        int iFirstSize = GetSize(first);
        int iSecondSizee = GetSize(second);
        int iLen = iFirstSize + iSecondSizee;
        if(iLen < 1)
            return null;
        byte[] retBytes = new byte[iLen];
        if (iFirstSize > 0)
        {
            System.arraycopy(first, 0, retBytes, 0, iFirstSize);
        }
        if (iSecondSizee>0)
        {
            System.arraycopy(second, 0, retBytes, iFirstSize, iSecondSizee);
        }
        return retBytes;
    }
    public static byte[] GetFirstBytes(byte[] srcBytes, int iFirstCount)
    {
        if(null == srcBytes)
            return null;
        if(iFirstCount < 1)
            return null;
        if(iFirstCount > srcBytes.length)
            iFirstCount = srcBytes.length;
        byte[] retBytes = new byte[iFirstCount];
        System.arraycopy(srcBytes, 0, retBytes, 0, iFirstCount);
        return retBytes;
    }
    // remove front bytes
    public static byte[] GetLastBytes(byte[] srcBytes, int iRemoveCount)
    {
        if(null == srcBytes)
            return null;
        if(iRemoveCount >= srcBytes.length)
            return null;
        if (iRemoveCount < 0) {
            iRemoveCount = 0;
        }
        int iLastCount = srcBytes.length - iRemoveCount;
        byte[] retBytes = new byte[iLastCount];
        System.arraycopy(srcBytes, iRemoveCount, retBytes, 0, iLastCount);
        return retBytes;
    }

}
