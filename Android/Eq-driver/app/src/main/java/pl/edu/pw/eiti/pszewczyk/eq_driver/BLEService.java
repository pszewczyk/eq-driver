package pl.edu.pw.eiti.pszewczyk.eq_driver;

import android.app.Service;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.util.Log;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.UUID;

/**
 * Created by severus on 14.05.17.
 */

public class BLEService extends Service {
    public static final String TAG = "BLeService";
    public static final String ACTION_GATT_CONNECTED = "pl.edu.pw.eiti.pszewczyk.ACTION_GATT_CONNECTED";
    public static final String ACTION_GATT_DISCONNECTED = "pl.edu.pw.eiti.pszewczyk.ACTION_GATT_DISCONNECTED";
    public static final String ACTION_GATT_SERVICES_DISCOVERED = "pl.edu.pw.eiti.pszewczyk.ACTION_GATT_SERVICES_DISCOVERED";
    public static final String ACTION_DATA_AVAILABLE = "pl.edu.pw.eiti.pszewczyk.ACTION_DATA_AVAILABLE";
    public static final int STATE_DISCONNECTED = 0;
    public static final int STATE_CONNECTED = 1;
    public static final short CHAR_POS_UUID = 0x1525;
    public static final short CHAR_DEST_UUID = 0x1526;
    public static final short CHAR_MODE_UUID = 0x1527;
    public static final short CHAR_REVERSE_UUID = 0x1527;
    public static String CLIENT_CHARACTERISTIC_CONFIG = "00002902-0000-1000-8000-00805f9b34fb";

    LinkedList<BluetoothGattCharacteristic> readQ = new LinkedList<BluetoothGattCharacteristic>();
    boolean reading = false;

    BluetoothDevice device;
    int mConnectionState = STATE_DISCONNECTED;
    public static BluetoothGatt mBluetoothGatt;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


    public BLEService( ) {
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Bundle extra = intent.getExtras();
        device = (BluetoothDevice) extra.get("device");
        Log.i(TAG, "created service for " + device.getName());

        device.createBond();
        mBluetoothGatt = device.connectGatt(this, true, mGattCallback);

        mBluetoothGatt.discoverServices();

        return super.onStartCommand(intent, flags, startId);
    }

    public static short getShortUUID(UUID uuid) {
        return (short)(uuid.getMostSignificantBits()>>32);
    }

    private final BluetoothGattCallback mGattCallback =
            new BluetoothGattCallback() {
                @Override
                public void onConnectionStateChange(BluetoothGatt gatt, int status,
                                                    int newState) {
                    String intentAction;
                    if (newState == BluetoothProfile.STATE_CONNECTED) {
                        Log.i(TAG, "connected");
                        mConnectionState = STATE_CONNECTED;
                        sendBroadcast(new Intent(ACTION_GATT_CONNECTED));
                        mBluetoothGatt.discoverServices();
                    } else if (newState == STATE_DISCONNECTED) {
                        mConnectionState = STATE_DISCONNECTED;
                        sendBroadcast(new Intent(ACTION_GATT_DISCONNECTED));
                        Log.i(TAG, "disconnected");
                        mBluetoothGatt.connect();
                    }
                }

                @Override
                // New services discovered
                public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                    if (status == BluetoothGatt.GATT_SUCCESS)
                        sendBroadcast(new Intent(ACTION_GATT_SERVICES_DISCOVERED));

                    Log.i(TAG, "discovered " + status);
                    List<BluetoothGattService> services = gatt.getServices();
                    for (BluetoothGattService service : services) {
                        Log.i(TAG, "Service: " + service.getUuid());
                        List<BluetoothGattCharacteristic> chars = service.getCharacteristics();
                        for (BluetoothGattCharacteristic characteristic: chars) {
                            short id = getShortUUID(characteristic.getUuid());
                            if (id == CHAR_DEST_UUID || id == CHAR_MODE_UUID || id == CHAR_POS_UUID || id == CHAR_REVERSE_UUID) {
                                readQ.add(characteristic);
                                mBluetoothGatt.setCharacteristicNotification(characteristic, true);
                                BluetoothGattDescriptor descriptor = characteristic.getDescriptor(
                                        UUID.fromString(CLIENT_CHARACTERISTIC_CONFIG));
                                descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                                mBluetoothGatt.writeDescriptor(descriptor);
                            }
                            Log.i(TAG, "Characteristic " + id);
                        }
                    }

                    if (!reading && !readQ.isEmpty()) {
                        BluetoothGattCharacteristic characteristic = readQ.pop();
                        reading = true;
                        mBluetoothGatt.readCharacteristic(characteristic);
                    }
                }

                @Override
                // Result of a characteristic read operation
                public void onCharacteristicRead(BluetoothGatt gatt,
                                                 BluetoothGattCharacteristic characteristic,
                                                 int status) {
                    if (status == BluetoothGatt.GATT_SUCCESS) {
                        sendBroadcast(new Intent(ACTION_DATA_AVAILABLE));
                        Log.i(TAG, "Read " + getShortUUID(characteristic.getUuid()));
                    }

                    if (readQ.isEmpty())
                        reading = false;
                    else {
                        reading = true;
                        BluetoothGattCharacteristic c = readQ.pop();
                        mBluetoothGatt.readCharacteristic(c);
                    }
                }

                @Override
                public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
                    sendBroadcast(new Intent(ACTION_DATA_AVAILABLE));
                    Log.i(TAG, "Updated " + getShortUUID(characteristic.getUuid()));
                    super.onCharacteristicChanged(gatt, characteristic);
                }
            };
}
