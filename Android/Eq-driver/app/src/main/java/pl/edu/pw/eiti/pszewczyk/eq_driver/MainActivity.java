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
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    private final static String TAG = "MainActivity";
    public static BluetoothDevice device;

    ArrayList<SphereCoord> knownObjects = new ArrayList<>();
    ArrayAdapter<SphereCoord> objectsAdapter;
    Spinner objectSpinner;
    TextView statusText;

    int targetRA, targetDec;
    boolean gotoSaved = false;
    boolean customTarget = false;

    void addKnownObjects()
    {
        String stars[] = getResources().getStringArray(R.array.stars);

        for (String star: stars) {
            knownObjects.add(new SphereCoord(star));
        }
    }

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

        statusText = (TextView) findViewById(R.id.statusText);

        objectSpinner = (Spinner) findViewById(R.id.starSpinner);
        addKnownObjects();
        knownObjects.add(new SphereCoord(0, 0, "Custom"));
        objectsAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, knownObjects);
        objectsAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        objectSpinner.setAdapter(objectsAdapter);

        objectSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                SphereCoord coord = objectsAdapter.getItem(position);

                if (coord.Name.equals("Custom")) {
                    showCustomInput(true);
                    customTarget = true;
                } else {
                    showCustomInput(false);
                    targetRA = coord.RA;
                    targetDec = coord.Dec;
                    customTarget = false;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                showCustomInput(false);
                customTarget = false;
            }
        });
    }

    void showCustomInput(boolean show)
    {
        int v;

        if (show)
            v = View.VISIBLE;
        else
            v = View.INVISIBLE;

        findViewById(R.id.customRaHour).setVisibility(v);
        findViewById(R.id.customRaMin).setVisibility(v);
        findViewById(R.id.customDecDeg).setVisibility(v);
        findViewById(R.id.customDecMin).setVisibility(v);
        findViewById(R.id.customlabel1).setVisibility(v);
        findViewById(R.id.customLabel2).setVisibility(v);
        findViewById(R.id.customLabel3).setVisibility(v);
        findViewById(R.id.customLabel4).setVisibility(v);
    }

    void do_update_pos(byte[] bytes)
    {
        if (bytes == null) {
            Log.e(TAG, "got null as position");
        }
        if (bytes.length < 8) {
            Log.e(TAG, "got only " + bytes.length + " bytes for position, aborting");
            return;
        }

        EditText radeg, ramin, decdeg, decmin;
        radeg = (EditText) findViewById(R.id.RAPosHour);
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

        SphereCoord coord = new SphereCoord(ra, dec, "");

        Log.i(TAG, "New position: " + ra + ", " + dec);

        radeg.setText(String.valueOf(coord.RAhour()));
        ramin.setText(String.valueOf(coord.RAmin()));
        decdeg.setText(String.valueOf(coord.Decdeg()));
        decmin.setText(String.valueOf((coord.Decmin())));
    }

    void do_update_dest(byte[] bytes)
    {
        if (bytes == null) {
            Log.e(TAG, "got null as dest");
            return;
        }
        if (bytes.length < 8) {
            Log.e(TAG, "got only " + bytes.length + " bytes for GoTo, aborting");
            return;
        }

        EditText radeg, ramin, decdeg, decmin;
        radeg = (EditText) findViewById(R.id.GotoRAHour);
        decdeg = (EditText) findViewById(R.id.GotoDecDeg);
        ramin = (EditText) findViewById(R.id.GotoRaMin);
        decmin = (EditText) findViewById(R.id.GotoDecMin);

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

        SphereCoord coord = new SphereCoord(ra, dec, "");

        Log.i(TAG, "New position: " + ra + ", " + dec);

        radeg.setText(String.valueOf(coord.RAhour()));
        ramin.setText(String.valueOf(coord.RAmin()));
        decdeg.setText(String.valueOf(coord.Decdeg()));
        decmin.setText(String.valueOf((coord.Decmin())));

        if (gotoSaved) {
            gotoSaved = false;
            byte mode_bytes[] = {BLEService.MODE_GOTO};
            BLEService.writeCharacteristic(BLEService.FULL_CHAR_MODE_UUID, mode_bytes);
        }
    }

    private final BroadcastReceiver mGattBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BLEService.ACTION_GATT_CONNECTED.equals(action)) {
                Log.i(TAG, "Connected");
                statusText.setText("Discovering...");
            } else if (BLEService.ACTION_GATT_DISCONNECTED.equals(action)) {
                Log.i(TAG, "Disconnected, attempting reconnection");
                if (!BLEService.mBluetoothGatt.connect()) {
                    Log.i(TAG, "Could not connect, aborting");
                    finish();
                }
                statusText.setText("Reconecting...");
            } else if (BLEService.ACTION_GATT_SERVICES_DISCOVERED.equals(action)) {
                Log.i(TAG, "Discovered");
                statusText.setText("Connected.");
            } else if (BLEService.ACTION_DATA_AVAILABLE.equals(action)) {
                List<BluetoothGattService> services = BLEService.mBluetoothGatt.getServices();
                BluetoothGattService service = BLEService.mBluetoothGatt.getService(
                        UUID.fromString(BLEService.MAIN_SERVICE_UUID));
                List<BluetoothGattCharacteristic> chars = service.getCharacteristics();
                BluetoothGattCharacteristic characteristic;
                byte[] value;

                characteristic = service.getCharacteristic(BLEService.FULL_CHAR_POS_UUID);
                value = characteristic.getValue();
                do_update_pos(value);
                characteristic = service.getCharacteristic(BLEService.FULL_CHAR_DEST_UUID);
                value = characteristic.getValue();
                do_update_dest(value);
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
        return super.onOptionsItemSelected(item);
    }

    byte[] getTargetBytes()
    {
        byte bytes[] = new byte[8];

        ByteBuffer rab = ByteBuffer.wrap(bytes, 0, 4);
        ByteBuffer decb = ByteBuffer.wrap(bytes, 4, 4);

        rab.order(ByteOrder.LITTLE_ENDIAN);
        decb.order(ByteOrder.LITTLE_ENDIAN);

        if (customTarget) {
            EditText rah, ramin, decdeg, decmin;
            rah = (EditText) findViewById(R.id.customRaHour);
            ramin = (EditText) findViewById(R.id.customRaMin);
            decdeg = (EditText) findViewById(R.id.customDecDeg);
            decmin = (EditText) findViewById(R.id.customDecMin);

            targetRA = 54000 * Integer.parseInt(rah.getText().toString())
                    + 900 * Integer.parseInt(ramin.getText().toString());
            targetDec = 3600 * Integer.parseInt(decdeg.getText().toString())
                    + 60 * Integer.parseInt(decmin.getText().toString());
        }

        rab.putInt(targetRA);
        decb.putInt(targetDec);

        return bytes;
    }

    public void doGoto(View view)
    {
        try {
            BLEService.writeCharacteristic(BLEService.FULL_CHAR_DEST_UUID, getTargetBytes());
            gotoSaved = true;
        } catch (Exception e) {
            Log.e(TAG, "goGoto exception: " + e.getMessage());
        }
    }

    public void doSync(View view)
    {
        try {
            BLEService.writeCharacteristic(BLEService.FULL_CHAR_POS_UUID, getTargetBytes());
        } catch (Exception e) {
            Log.e(TAG, "doSync exception: " + e.getMessage());
        }
    }
}
