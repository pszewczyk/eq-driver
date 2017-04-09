package pl.edu.pw.eiti.pszewczyk.eq_driver;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    private BluetoothAdapter btAdapter;
    private BluetoothDevice mDevice;
    private boolean scanning;

    private static final long SCANNING_TIMEOUT = 10000;

    private BluetoothAdapter.LeScanCallback mLeScanCallback =
            new BluetoothAdapter.LeScanCallback() {
                @Override
                public void onLeScan(final BluetoothDevice device, int rssi,
                                     byte[] scanRecord) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            TextView title;
                            title = (TextView) findViewById(R.id.textTitle);
                            mDevice = device;
                            title.setText(device.getName());
                        }
                    });
                }
            };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        final BluetoothManager btm = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btm.getAdapter();
/*
    TODO: ask user for bluetooth enable
        if (!btAdapter.isEnabled()) {
            Intent enabledIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enabledIntent, REQUEST_ENABLE_BT);
        }
           */
    }

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

    public void doScanTrigger(View view) {
        Button b;
        b = (Button)(findViewById(R.id.scanButton));

        if (scanning) {
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

}
