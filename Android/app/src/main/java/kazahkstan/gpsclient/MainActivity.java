/*-----------------------------------------------------------------------
--  SOURCE FILE:    MainActivity.java   An application that forwards location data to a user-specified
--                                      server
--
--  PROGRAM:        GPS client
--
--  FUNCTIONS:
--                  protected void onCreate(Bundle savedInstanceState)
--                  public String getLocalIpAddress()
--                  public void startConnection(View view)
--                  protected void getData(Location location)
--                  protected void sendData(String data)
--                  protected TCPclient doInBackground(String... params)
--
--  DATE:           March 24, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  NOTES:
--  The program prompts the user to enter in the server IP and Port Number.
--  Then, once the user presses the connect button, attempts to connect and starts
--  sending GPS data to the server over the socket
----------------------------------------------------------------------------*/
package kazahkstan.gpsclient;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;


public class MainActivity extends AppCompatActivity {
    private static final int MIN_DELAY = 30000;  //the minimum amount of time to wait between updates in milliseconds
    private static final int MIN_DISTANCE = 20; //the minimum distance that must be covered between updates in meteres

    EditText ip;
    EditText port;
    String ipAddress;
    int servPort;
    LocationManager locationManager;
    String localIP;
    String deviceName;
    TCPclient tcp;

    //prepare location listener for gps updates
    LocationListener locationListener = new LocationListener() {
        //defines the callback function executed upon a location change
        public void onLocationChanged(Location location) {
            getData(location);
        }
        //required
        public void onStatusChanged(String provider, int status, Bundle extras) {}
        //required
        public void onProviderEnabled(String provider) {}
        //required
        public void onProviderDisabled(String provider) {}
    };



/*---------------------------------------------------------------------------------
--  FUNCTION:       onCreate
--
--  DATE:           March 20, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      void onCreate(Bundle savedInstanceState)
--                  savedInstanceState: the status of the last closed instance of the app
--
--  RETURNS:        void
--
--  NOTES:
--  The standard android onCreate method required in every activity. Has been modified
--  to include the require permissions, define our input boxes, provide us with a
--  location manager and retrieve our IP address
-----------------------------------------------------------------------------------*/
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.ACCESS_FINE_LOCATION},1);
        ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.INTERNET},1);
        ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.ACCESS_WIFI_STATE},1);
        ip   = (EditText) findViewById(R.id.ipBox);
        port = (EditText) findViewById(R.id.portBox);
        locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        localIP = getLocalIpAddress();
        deviceName = android.os.Build.MODEL;
    }

/*---------------------------------------------------------------------------------
--  FUNCTION:       getLocalIpAddress
--
--  DATE:           March 24, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      String getLocalIpAddress()
--
--  RETURNS:        a string containing the device's IP address in dotted decimal notation
--
--  NOTES:
--  A simple method that queries the systems wifi manager to retrieve connection info, then
--  parses it to a human readable format.
-----------------------------------------------------------------------------------*/
    public String getLocalIpAddress(){
        WifiManager wifiMan = (WifiManager) this.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        WifiInfo wifiInf = wifiMan.getConnectionInfo();
        int ipAddress = wifiInf.getIpAddress();
        String ip = String.format("%d.%d.%d.%d", (ipAddress & 0xff),(ipAddress >> 8 & 0xff),(ipAddress >> 16 & 0xff),(ipAddress >> 24 & 0xff));
        return ip;
    }

/*---------------------------------------------------------------------------------
--  FUNCTION:       startConnection
--
--  DATE:           March 21, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      void startConnection(View view)
--                  view: the button that was clicked to trigger this method.
--
--  RETURNS:        void
--
--  NOTES:
--  The callback function triggered when connect is clicked. It parses out the text
--  entered by the user, and checks to make sure it's in the right format. It then starts
--  the connectTask to open the connection. While the connection is being set up,
--  it makes a request to the location manager to start listening for GPS updates
-----------------------------------------------------------------------------------*/
    public void startConnections(View view) {

        ipAddress = ip.getText().toString();
        String portNum   = port.getText().toString();

        if (ipAddress.equals("")) {
            Toast.makeText(this,"Invalid IP, please try again",Toast.LENGTH_SHORT).show();
        }
        try {
            servPort = Integer.parseInt(portNum);
        } catch (NumberFormatException e) {
            Toast.makeText(this,"Invalid Port, please try again",Toast.LENGTH_SHORT).show();
            return;
        }
        //attempt to connect
        new connectTask().execute("");

        //start gps reading
        if (ContextCompat.checkSelfPermission(getApplicationContext(),Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED){
            locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, MIN_DELAY, MIN_DISTANCE, locationListener);
            locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, MIN_DELAY, MIN_DISTANCE, locationListener);
        }
    }

/*---------------------------------------------------------------------------------
--  FUNCTION:       utcToString
--
--  DATE:           March 27, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      String utcToString(long time)
--                  time: the location returned by a GPS update
--
--  RETURNS:        a String representing the time in GMT
--
--  NOTES:
--  This function exists to convert the time from utc format to something more human readable.
-----------------------------------------------------------------------------------*/
public String utcToString(long time) {

    SimpleDateFormat sdf = new SimpleDateFormat("dd/MMM/yyyy-HH:mm:ss");
    String timestamp = sdf.format(time);
    return timestamp;
}


/*---------------------------------------------------------------------------------
--  FUNCTION:       getData
--
--  DATE:           March 23, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      getData(Location location)
--                  location: the location returned by a GPS update
--
--  RETURNS:        void
--
--  NOTES:
--  The callback function triggered when location updates. It translates the information
--  returned into the format requested by the server. It then checks to see if the
--  socket has been set-up, before attempting to send the gps data to the server.
-----------------------------------------------------------------------------------*/
    protected void getData(Location location) {
        String timestamp = utcToString(location.getTime());
        String gpsData = timestamp + " " + localIP + " " + deviceName + " " + location.getLatitude() + " " + location.getLongitude();
//        Toast.makeText(this,gpsData,Toast.LENGTH_LONG).show();
    if (tcp.gpsON != false)
        try {
            sendData(gpsData);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

/*---------------------------------------------------------------------------------
--  FUNCTION:       sendData
--
--  DATE:           March 23, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      sendData(String data)
--                  data: the formatted string containing the desired data for the server.
--
--  RETURNS:        void
--
--  NOTES:
--  The function which sends data to the server. It throws an IO exception if the socket
--  is unusable for whatever reason.
-----------------------------------------------------------------------------------*/
    protected void sendData(String data) throws IOException {
        PrintWriter out = new PrintWriter(tcp.client.getOutputStream(),true);
        out.println(data);
    }

/*---------------------------------------------------------------------------------
--  FUNCTION:       connectTask
--
--  DATE:           March 23, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      protected TCPclient doInBackground(String params)
--                  params: any parameters required by the methods called within the function.
--
--  RETURNS:        null
--
--  NOTES:
--  A helper function required due to how android handles network requests. All network
--  requests must run on a separate thread that is run in the background. The AsyncTask
--  template is the preferred method of achieving this according to the documentation
--  I have read. The only method within is one that simply starts its own thread with
--  a specified class and method.
-----------------------------------------------------------------------------------*/
    public class connectTask extends AsyncTask<String, String, TCPclient> {
        @Override
        protected TCPclient doInBackground(String... params) {
            tcp = new TCPclient(ipAddress,servPort);
            tcp.run(locationManager,locationListener);
            return null;
        }
    }
}
