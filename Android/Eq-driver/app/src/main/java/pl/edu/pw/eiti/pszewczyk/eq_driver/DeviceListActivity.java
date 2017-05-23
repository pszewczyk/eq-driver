package pl.edu.pw.eiti.pszewczyk.eq_driver;

import android.Manifest;
import android.app.ListActivity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.UUID;

public class DeviceListActivity extends ListActivity {
    ArrayList<String> listItems = new ArrayList<String>();
    ArrayList<BluetoothDevice> listDevs = new ArrayList<BluetoothDevice>();
    ArrayAdapter<String> arrayAdapter;
    public static final String EXTRA_MESSAGE = "pl.edu.pw.eiti.pszewczyk.MESSAGE";

    private BluetoothAdapter btAdapter;
    private boolean scanning;
    private final static int REQUEST_ENABLE_BT = 1;
    private final static int REQUEST_ENABLE_COARSE_LOCATION = 2;
    private final static int REQUEST_ENABLE_FINE_LOCATION = 3;

    private static final long SCANNING_TIMEOUT = 10000;

    private BluetoothAdapter.LeScanCallback mLeScanCallback =
            new BluetoothAdapter.LeScanCallback() {
                @Override
                public void onLeScan(final BluetoothDevice device, int rssi,
                                     byte[] scanRecord) {
                    if (!listDevs.contains(device)) {
                        listItems.add(device.getName());
                        listDevs.add(device);
                        arrayAdapter.notifyDataSetChanged();
                    }
                }
            };

    private void ensurePriv(String priv, int req)
    {
        if (ContextCompat.checkSelfPermission(this, priv)
                != PackageManager.PERMISSION_GRANTED) {

            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    priv)) {
            } else {
                ActivityCompat.requestPermissions(this,
                        new String[]{priv},
                        REQUEST_ENABLE_COARSE_LOCATION);
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_list);
            arrayAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1,
                listItems);
        setListAdapter(arrayAdapter);
        arrayAdapter.notifyDataSetChanged();

        ensurePriv(Manifest.permission.ACCESS_COARSE_LOCATION, REQUEST_ENABLE_COARSE_LOCATION);
        ensurePriv(Manifest.permission.ACCESS_FINE_LOCATION, REQUEST_ENABLE_FINE_LOCATION);

        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            /* TODO Handle unsupported ble */
        }

        final BluetoothManager btm = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btm.getAdapter();
        if (!btAdapter.isEnabled()) {
            Intent enabledIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enabledIntent, REQUEST_ENABLE_BT);
        }
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);
        MainActivity.device = listDevs.get(position);
        Intent intent = new Intent(this, MainActivity.class);
        startActivity(intent);
    }

    public void setScan(boolean enable)
    {
        Button b;
        b = (Button)(findViewById(R.id.scanBtn));

        if (!enable) {
            b.setText("Start scan");
            scanning = false;
            btAdapter.stopLeScan(mLeScanCallback);
        }

        else {
            b.setText("Stop scan");
            scanning = true;
            btAdapter.startLeScan(mLeScanCallback);

        }
    }

    public void doScanTrigger(View view) {
        if (scanning) {
            setScan(false);
        }
        else {
            setScan(true);
        }
    }
}
