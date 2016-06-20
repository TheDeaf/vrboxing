package oculus;

/**
 * Defines several constants used between {@link BluetoothChatService} and the UI.
 */
public interface Constants {

    // Message types sent from the BluetoothChatService Handler
    public static final int MESSAGE_STATE_CHANGE = 1;
    public static final int MESSAGE_READ = 2;
    public static final int MESSAGE_WRITE = 3;
    public static final int MESSAGE_DEVICE_NAME = 4;
    public static final int MESSAGE_TOAST = 5;

    // Key names received from the BluetoothChatService Handler
    public static final String DEVICE_NAME = "device_name";
    public static final String TOAST = "toast";

    // Message types sent from BoxingDataUtil Handler
    public static final int MESSAGE_BOXING_START = 1;
    public static final int MESSAGE_BOXING_END = 2;

    // key names receive from the BoxingDataUtil Handler
    public static final String HitMaxValue = "hit_value";
    public static final String MoveSpeed = "move_speed";
}
