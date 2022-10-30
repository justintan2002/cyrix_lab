from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QPushButton, QLabel, QVBoxLayout, QStackedWidget
from PySide6.QtGui import QPixmap, QFont, QFontDatabase
from PySide6.QtCore import Qt, QSize, QObject, QIODevice, Signal, Slot
from PySide6.QtSerialPort import QSerialPort, QSerialPortInfo
import sys

from rospy import loginfo

class Login_Page(QWidget):
    def __init__(self):
        super().__init__()

        self.label_connect = QLabel()
        self.pixmap_connect = QPixmap("connect.png")
        self.pixmap_connect = self.pixmap_connect.scaledToHeight(300)
        self.label_connect.setPixmap(self.pixmap_connect)
        self.label_connect.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        self.label_word = QLabel("PIXIIE AUTHENTICATION")
        self.label_word.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        QFontDatabase.addApplicationFont("Anurati-Regular.otf")
        #print(QFontDatabase.families(QFontDatabase.Latin))
       
        label_font = QFont("Anurati", pointSize=30, weight=1)
        self.label_word.setFont(label_font)
        #button = QPushButton("Press Me!")
        
        self.layout = QVBoxLayout()
        self.layout.addWidget(self.label_connect)
        self.layout.addWidget(self.label_word)

        self.setLayout(self.layout)

class Cyrix_Page(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setGeometry(0, 0, 1000, 1000)
        self.setWindowTitle("Cyrix Lab")

class Serial_Interface(QObject):
    auth_signal = Signal()
    def __init__(self):
        super().__init__()
        self.port = QSerialPort()
    
    def open(self):
        print("Scanning Port ...")
        port_list = QSerialPortInfo().availablePorts()
        [print(port.portName(), end=" ") for port in port_list]
        self.port.setBaudRate(QSerialPort.BaudRate.Baud115200)
        self.port.setPortName("COM7")
        self.port.setParity(QSerialPort.Parity.NoParity)
        self.port.setStopBits(QSerialPort.StopBits.OneStop)
        self.port.setDataBits(QSerialPort.DataBits.Data8)
        self.port.setFlowControl(QSerialPort.FlowControl.NoFlowControl)
        self.port.readyRead.connect(self.read)
        s = self.port.open(QIODevice.ReadWrite)
        if s:
            print("\nPort Opened")
        else:
            print("\nPort Failed to Open")

    def close(self):
        self.port.close()

    def read(self):
        while(self.port.canReadLine()):
            msg = self.port.readLine()
            decoded_msg = msg.toStdString()
            print(decoded_msg, end='')
            if decoded_msg.strip('\n') == "Auth: 12345":
                self.auth_signal.emit()

    def write(self):
        pass

class main_window(QMainWindow):
    def __init__(self):
        super().__init__()
        self.screen = QStackedWidget()
        self.login_page = Login_Page()
        self.cyrix_page = Cyrix_Page()
        self.screen.addWidget(self.login_page)
        self.screen.addWidget(self.cyrix_page)
        self.screen.setCurrentWidget(self.login_page)
        self.setCentralWidget(self.screen)

        self.ser = Serial_Interface()
        self.ser.auth_signal.connect(self.switch_screen)

    
    def switch_screen(self):
        self.screen.setCurrentWidget(self.cyrix_page)
        print("switch")
        

def main():
    app = QApplication(sys.argv)
    m = main_window()
    m.ser.open()
    m.show()
    
    app.exec()

if __name__ == "__main__":
    main()
