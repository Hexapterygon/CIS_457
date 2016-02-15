import java.io.*;
import java.net.*;

class udpclient{
    public static void main(String args[]) throws Exception{
        DatagramSocket clientSocket = new DatagramSocket();
        BufferedReader inFromUser = 
            new BufferedReader(new InputStreamReader(System.in));
        System.out.println("Enter a message: ");
        String message = inFromUser.readLine();
        byte[] sendData = message.getBytes();
        InetAddress IPAddress = InetAddress.getByName("127.0.0.1");
        int port = 9876;
        DatagramPacket sendPacket = 
            new DatagramPacket(sendData, sendData.length, IPAddress, port);
        clientSocket.send(sendPacket);
        System.out.println("Ok, I sent it");

    }

}
