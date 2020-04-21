package ro.digitallabs.catoyremote

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*
import java.io.IOException
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.net.SocketException
import kotlin.concurrent.thread
import kotlin.math.*


class MainActivity : AppCompatActivity() {

    private var x = MainActivity.MID_SERVO
    private var y = MainActivity.MID_SERVO
    private var listeningForBroadcast = false
    private var listeningThread: Thread? = null

    private var isSendingMessages = false
    private var sendingThread: Thread? = null

    private var toyIP: InetAddress? = null
    private var toyPort: Int? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        joystickView.setOnMoveListener { angle, strength ->
            val nextX =
                MainActivity.MID_SERVO + (MainActivity.MAX_SERVO - MainActivity.MIN_SERVO) / 2 * cos(
                    Math.toRadians(angle.toDouble())
                ) * strength / 100
            val nextY =
                MainActivity.MID_SERVO + (MainActivity.MAX_SERVO - MainActivity.MIN_SERVO) / 2 * sin(
                    Math.toRadians(angle.toDouble())
                ) * strength / 100
            this.x = min(MAX_SERVO, max(MIN_SERVO, nextX))
            this.y = min(MAX_SERVO, max(MIN_SERVO, nextY))
            Log.wtf("JOYSTICK", "${x.roundToInt()}  - ${y.roundToInt()}")
        }
    }

    private fun startSendingMessages() {
        isSendingMessages = true
        sendingThread = thread {
            val udpSocket = DatagramSocket()
            while (isSendingMessages) {
                toyIP?.let { toyIP ->
                    toyPort?.let { toyPort ->
                        try {
                            val buf = "${x.roundToInt()}/${y.roundToInt()}".toByteArray()
                            val packet = DatagramPacket(buf, buf.size, toyIP, toyPort)
                            udpSocket.send(packet)
                        } catch (e: SocketException) {
                            Log.e("UDP", e.message)
                        } catch (e: IOException) {
                            Log.e("UDP", e.message)
                        }
                    }
                }
                Thread.sleep(50)
            }
            udpSocket.close()
        }
    }

    private fun stopSendingMessages() {
        isSendingMessages = false
        try {
            sendingThread?.join(100)
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
        sendingThread = null
    }


    private fun broadcastReceived(message: String, remoteAddress: InetAddress) {
        if (message.startsWith("CaToy/")) {
            message.replace("CaToy/", "").toIntOrNull()?.let {
                toyPort = it
                toyIP = remoteAddress
            }
        }
    }

    private fun listenForBroadcast() {
        listeningForBroadcast = true
        listeningThread = thread {
            val socket = DatagramSocket(MainActivity.UDP_LISTEN_PORT)
            while (listeningForBroadcast) {
                try {
                    val message = ByteArray(8000)
                    val packet = DatagramPacket(message, message.size)
                    socket.receive(packet)
                    val text = String(message, 0, packet.length)
                    runOnUiThread {
                        broadcastReceived(text, packet.address)
                    }
                } catch (e: IOException) {
                    listeningForBroadcast = false
                    Log.e("UDP", e.message)
                }
            }
            socket.close()
        }
    }

    private fun stopListeningForBroadcast() {
        listeningForBroadcast = false
        try {
            listeningThread?.join(100)
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
        listeningThread = null
    }


    override fun onResume() {
        super.onResume()
        listenForBroadcast()
        startSendingMessages()
    }

    override fun onPause() {
        stopListeningForBroadcast()
        stopSendingMessages()
        super.onPause()

    }

    companion object {
        const val MIN_SERVO = 20.0
        const val MAX_SERVO = 160.0
        const val MID_SERVO = (MainActivity.MIN_SERVO + MainActivity.MAX_SERVO) / 2
        const val UDP_LISTEN_PORT = 19999

        const val MAXIMUM_CHANGE = 5
    }
}