/*-----------------------------------------------------------------------
--  SOURCE FILE:    TCPclient.java   A sub-class created to handle setting up connections
--
--  PROGRAM:        GPS client
--
--  FUNCTIONS:
--                  public TCPclient(String address, int servPort)
--                  public void run(LocationManager locationManager, LocationListener locationListener)
--
--  DATE:           March 24, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  NOTES:
--  This is a sub-class created to handle the actual task of establishing a connection.
--  The class is requried due to android conventions preventing networking operations
--  from occuring on the main thread.
----------------------------------------------------------------------------*/
package kazahkstan.gpsclient;

import android.location.LocationListener;
import android.location.LocationManager;
import android.util.Log;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

public class TCPclient {
    public static String IP;
    public static int port;

    public boolean gpsON;
    public Socket client;

/*---------------------------------------------------------------------------------
--  FUNCTION:       TCPclient
--
--  DATE:           March 24, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      TCPclient onCreate(String address, int servPort)
--                  address: the Ip of the server in dotted Decimal notation
--                  servPort: the port you intend to connect to on the server
--
--  RETURNS:        A TCPclient object
--
--  NOTES:
--  The constructor for the tcp client that defines the target server and port
-----------------------------------------------------------------------------------*/
    public TCPclient(String address, int servPort) {
        IP = address;
        port = servPort;
    }
/*---------------------------------------------------------------------------------
--  FUNCTION:       run
--
--  DATE:           March 24, 2017
--
--  DESIGNER:       Matt Goerwell
--
--  PROGRAMMER:     Matt Goerwell
--
--  INTERFACE:      void run(LocationManager locationManager, LocationListener locationListener)
--                  locationManager: the manager from the main class
--                  locationListener: the listener from the main class
--
--  RETURNS:        void
--
--  NOTES:
--  The method that actually establishes the connection. it converts the IP to a usable
--  state, then attempts to connect to the server. If it succeeds, it enters a loop that
--  maintains the socket until GPS operations are disabled by the user. Once the loop
--  exits, it then closes the socket and exits.
-----------------------------------------------------------------------------------*/
    public void run(LocationManager locationManager, LocationListener locationListener) {
        InetAddress serverAddr = null;
        //convert IP
        try {
            serverAddr = InetAddress.getByName(IP);
        } catch (UnknownHostException e) {
            Log.e("TCPclient","UnknownHost");
            return;
        }

        //atempt to connect
        try {
            client = new Socket(serverAddr,port);
        } catch (IOException e) {
            Log.e("TCPclient","SocketIOException");
            return;
        }

        gpsON = true;
        //loop that maintains the socket until GPS is done.
        while (gpsON) {
            if (!(locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))) {
                gpsON = false;
                locationManager.removeUpdates(locationListener);
            }
        }

        //close socket
        try {
            client.close();
        } catch (IOException e) {
            Log.e("TCPclient","Socket didn't close");
            return;
        }
    }

}
