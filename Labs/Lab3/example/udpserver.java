import java.io.*;
import java.net.*;

class udpserver{
    public static void main(String args[]) throws Exception{
        DatagramSocket serverSocket = new DatagramSocket(9876);
        while(true){
            byte[] receiveData = new byte[1024];
            DatagramPacket receivePacket = 
                new DatagramPacket(receiveData, receiveData.length);
            serverSocket.receive(receivePacket);
            String message = new String(receiveData);
            System.out.println("Got message " + message + " from client " + receivePacket.getAddress());

        }
    }
}
