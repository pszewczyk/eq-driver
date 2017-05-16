package pl.edu.pw.eiti.pszewczyk.eq_driver;

import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;
import android.widget.TextView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private final static String TAG = "MainActivity";
    public static BluetoothDevice device;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tw = (TextView)findViewById(R.id.textTitle);
        tw.setText(device.getName());


        IntentFilter filter = new IntentFilter();
        filter.addAction(BLEService.ACTION_DATA_AVAILABLE);
        filter.addAction(BLEService.ACTION_GATT_CONNECTED);
        filter.addAction(BLEService.ACTION_GATT_DISCONNECTED);
        filter.addAction(BLEService.ACTION_GATT_SERVICES_DISCOVERED);
        this.registerReceiver(mGattBroadcastReceiver, filter);

        Intent serviceIntent = new Intent(this, BLEService.class);
        serviceIntent.putExtra("device", device);
        startService(serviceIntent);
    }

    void do_update_pos(byte[] bytes)
    {
        EditText radeg, ramin, decdeg, decmin;
        radeg = (EditText) findViewById(R.id.RAPosDeg);
        decdeg = (EditText) findViewById(R.id.DECPosDeg);
        ramin = (EditText) findViewById(R.id.RAPosMin);
        decmin = (EditText) findViewById(R.id.DECPosMin);

        if (radeg == null)
            Log.e(TAG, "RA position deg field not found");
        if (decdeg == null)
            Log.e(TAG, "DEC position deg field not found");
        if (ramin == null)
            Log.e(TAG, "RA position min field not found");
        if (decmin == null)
            Log.e(TAG, "DEC position min field not found");

        ByteBuffer rab = ByteBuffer.wrap(bytes, 0, 4);
        ByteBuffer decb = ByteBuffer.wrap(bytes, 4, 4);

        rab.order(ByteOrder.LITTLE_ENDIAN);
        decb.order(ByteOrder.LITTLE_ENDIAN);

        int ra = rab.getInt();
        int dec = decb.getInt();

        Log.i(TAG, "New position: " + ra + ", " + dec);

        radeg.setText(String.valueOf(ra/3600));
        ramin.setText(String.valueOf((ra/60)%60));
        decdeg.setText(String.valueOf(dec/3600));
        decmin.setText(String.valueOf((dec/60)%60));
    }

    private final BroadcastReceiver mGattBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BLEService.ACTION_GATT_CONNECTED.equals(action)) {
                Log.i(TAG, "Connected");
            } else if (BLEService.ACTION_GATT_DISCONNECTED.equals(action)) {
                Log.i(TAG, "Disconnected");
            } else if (BLEService.ACTION_GATT_SERVICES_DISCOVERED.equals(action)) {
                Log.i(TAG, "Discovered");
            } else if (BLEService.ACTION_DATA_AVAILABLE.equals(action)) {
                List<BluetoothGattService> services = BLEService.mBluetoothGatt.getServices();
                for (BluetoothGattService service : services) {
                    Log.i(TAG, "Service: " + service.getUuid());
                    List<BluetoothGattCharacteristic> chars = service.getCharacteristics();
                    for (BluetoothGattCharacteristic characteristic: chars) {
                        short id = BLEService.getShortUUID(characteristic.getUuid());
                        BLEService.mBluetoothGatt.setCharacteristicNotification(characteristic, true);
                        Log.i(TAG, "Characteristic " + id);

                        if (id == BLEService.CHAR_POS_UUID) {
                            do_update_pos(characteristic.getValue());
                        }
                    }
                }

                Log.i(TAG, "Data available");
            }
        }
    };

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    public void doGoto(View view) {
        EditText gotoradeg, radeg;
        gotoradeg = (EditText) findViewById(R.id.GotoRADeg);
        radeg = (EditText) findViewById(R.id.RAPosDeg);

        radeg.setText(gotoradeg.getText());
    }
}
