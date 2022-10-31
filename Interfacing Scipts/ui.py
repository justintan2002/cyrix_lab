from PySide6.QtWidgets import *
from PySide6.QtGui import *
from PySide6.QtCore import *
from PySide6.QtSerialPort import *
import sys
import re

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
        
        self.layout = QVBoxLayout()
        self.layout.addWidget(self.label_connect)
        self.layout.addWidget(self.label_word)

        self.setLayout(self.layout)

class Cyrix_Page(QWidget):
    recharge_signal = Signal(str)
    cmode_signal = Signal(str)
    def __init__(self):
        self.threshold = [[900, 1200], [87, 110], [-1, 10], [0, 0.6], [20, 35], [5, 15]]
        self._charge_data = 0
        self.data1 = [
            ["Pressure/hPa", "Humidity/%rH", "Gyro/dps"],
            ["0", "0", "0"]
        ]
        self.data2 = [
            ["Magneto/gauss", "Temperature/C", "Accelero/ms2"],
            ["0", "0", "0"]
        ]
        super().__init__()
        self.setSizePolicy(QSizePolicy.Policy.MinimumExpanding, QSizePolicy.Policy.MinimumExpanding)
        self.table1 = QTableWidget()
        self.table2 = QTableWidget()
        self.table1.setAlternatingRowColors(True)
        self.table1.setSizeAdjustPolicy(QAbstractScrollArea.SizeAdjustPolicy.AdjustToContents)
        self.table1.resizeColumnsToContents()
        self.table1.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.table1.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.table1.verticalHeader().setVisible(False)
        self.table1.horizontalHeader().setVisible(False)
        self.table2.setAlternatingRowColors(True)
        self.table2.setSizeAdjustPolicy(QAbstractScrollArea.SizeAdjustPolicy.AdjustToContents)
        self.table2.resizeColumnsToContents()
        self.table2.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.table2.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.table2.verticalHeader().setVisible(False)
        self.table2.horizontalHeader().setVisible(False)
        self.table1.setRowCount(2)
        self.table1.setColumnCount(3)
        self.table2.setRowCount(2)
        self.table2.setColumnCount(3)
        # self.table1.setItem(0, 0, QTableWidgetItem(self.data1[0][0]))
        # self.table1.setItem(0, 1, QTableWidgetItem(self.data1[0][1]))
        # self.table1.setItem(0, 2, QTableWidgetItem(self.data1[0][2]))
        # self.table1.setItem(1, 0, QTableWidgetItem(self.data1[1][0]))
        # self.table1.setItem(1, 1, QTableWidgetItem(self.data1[1][1]))
        # self.table1.setItem(1, 2, QTableWidgetItem(self.data1[1][2]))
        # self.table2.setItem(0, 1, QTableWidgetItem(self.data2[0][1]))
        # self.table2.setItem(0, 2, QTableWidgetItem(self.data2[0][2]))
        # self.table2.setItem(0, 0, QTableWidgetItem(self.data2[0][0]))
        # self.table2.setItem(1, 0, QTableWidgetItem(self.data2[1][0]))
        # self.table2.setItem(1, 1, QTableWidgetItem(self.data2[1][1]))
        # self.table2.setItem(1, 2, QTableWidgetItem(self.data2[1][2]))
        for i in range(2):
            for j in range(3):
                self.table1.setItem(i, j, QTableWidgetItem(self.data1[i][j]))
                self.table2.setItem(i, j, QTableWidgetItem(self.data2[i][j]))
                self.table1.item(i, j).setTextAlignment(Qt.AlignmentFlag.AlignCenter)
                self.table2.item(i, j).setTextAlignment(Qt.AlignmentFlag.AlignCenter)

        # self.model1 = TableModel(self.data1)
        # self.model2 = TableModel(self.data2)
        # self.table1.setModel(self.model1)
        # self.table2.setModel(self.model2)
        self.charge_bar = QProgressBar()
        self.charge_bar.setMaximum(10)
        self.charge_bar.setValue(self._charge_data)
        self.charge_bar.setTextVisible(True)
        self.charge_bar.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.charge_bar.setFormat("Fluxer Battery %p%")

        self.charge_button = QPushButton("Charge", self)
        self.charge_button.clicked.connect(self.recharge)

        self.main_layout = QVBoxLayout()
        self.charge_combo = QWidget()
        self.charge_layout = QHBoxLayout()
        self.charge_layout.addWidget(self.charge_bar)
        self.charge_layout.addWidget(self.charge_button)
        self.charge_combo.setLayout(self.charge_layout)

        self.mode_button = QPushButton("Mode")
        self.mode_button.clicked.connect(self.change_mode)

        self.main_layout.addWidget(self.table1, alignment=Qt.AlignmentFlag.AlignCenter)
        self.main_layout.addWidget(self.table2, alignment=Qt.AlignmentFlag.AlignCenter)
        self.main_layout.addWidget(self.charge_combo, alignment = Qt.AlignmentFlag.AlignVCenter)
        self.main_layout.addWidget(self.mode_button, alignment=Qt.AlignmentFlag.AlignCenter)
        self.setLayout(self.main_layout)
    
    def recharge(self):
        self.recharge_signal.emit("00000")

    def change_mode(self):
        self.cmode_signal.emit('11111')

    def update_charge(self, data):
        self._charge_data = data
        self.charge_bar.setValue(self._charge_data)
    
    def update_mode(self, data):
        self.mode_button.setText(data+" Mode")

    def update_data(self, data):
        # s = self.table1.indexAt(QPoint(0, 1))
        # e = self.table1.indexAt(QPoint(2, 1))
        self.table1.setItem(1, 0, QTableWidgetItem(str(data[0])))
        if (data[0] > self.threshold[0][0] and data[0] < self.threshold[0][1]):
            self.table1.item(1, 0).setBackground(QColor(0, 200, 10, 150))
            self.table1.item(0, 0).setBackground(QColor(0, 200, 10, 150))
        else:
            self.table1.item(1, 0).setBackground(QColor(200, 0, 10, 150))
            self.table1.item(0, 0).setBackground(QColor(200, 0, 10, 150))
        self.table1.setItem(1, 1, QTableWidgetItem(str(data[1])))
        if (data[1] > self.threshold[1][0] and data[1] < self.threshold[1][1]):
            self.table1.item(1, 1).setBackground(QColor(0, 200, 10, 150))
            self.table1.item(0, 1).setBackground(QColor(0, 200, 10, 150))
        else:
            self.table1.item(1, 1).setBackground(QColor(200, 0, 10, 150))
            self.table1.item(0, 1).setBackground(QColor(200, 0, 10, 150))
        self.table1.setItem(1, 2, QTableWidgetItem(str(data[2])))
        if (data[2] > self.threshold[2][0] and data[2] < self.threshold[2][1]):
            self.table1.item(1, 2).setBackground(QColor(0, 200, 10, 150))
            self.table1.item(0, 2).setBackground(QColor(0, 200, 10, 150))
        else:
            self.table1.item(1, 2).setBackground(QColor(200, 0, 10, 150))
            self.table1.item(0, 2).setBackground(QColor(200, 0, 10, 150))
        self.table2.setItem(1, 0, QTableWidgetItem(str(data[3])))
        if (data[3] > self.threshold[3][0] and data[3] < self.threshold[3][1]):
            self.table2.item(1, 0).setBackground(QColor(0, 200, 10, 150))
            self.table2.item(0, 0).setBackground(QColor(0, 200, 10, 150))
        else:
            self.table2.item(1, 0).setBackground(QColor(200, 0, 10, 150))
            self.table2.item(0, 0).setBackground(QColor(200, 0, 10, 150))
        if len(data)==4:
            self.table2.setItem(1, 1, QTableWidgetItem(0))
            self.table2.setItem(1, 1, QTableWidgetItem(2))
            self.table2.item(1, 1).setBackground(QColor(0, 0, 0, 255))
            self.table2.item(1, 2).setBackground(QColor(0, 0, 0, 255))
            self.table2.item(0, 1).setBackground(QColor(255, 255, 255, 255))
            self.table2.item(0, 2).setBackground(QColor(255, 255, 255, 255))
        else:
            self.table2.setItem(1, 1, QTableWidgetItem(str(data[4])))
            if (data[4] > self.threshold[4][0] and data[4] < self.threshold[4][1]):
                self.table2.item(1, 1).setBackground(QColor(0, 200, 10, 150))
                self.table2.item(0, 1).setBackground(QColor(0, 200, 10, 150))
            else:
                self.table2.item(1, 1).setBackground(QColor(200, 0, 10, 150))
                self.table2.item(0, 1).setBackground(QColor(200, 0, 10, 150))
            self.table2.setItem(1, 2, QTableWidgetItem(str(data[5])))
            if (data[5] > self.threshold[5][0] and data[5] < self.threshold[5][1]):
                self.table2.item(1, 2).setBackground(QColor(0, 200, 10, 150))
                self.table2.item(0, 2).setBackground(QColor(0, 200, 10, 150))
            else:
                self.table2.item(1, 2).setBackground(QColor(200, 0, 10, 150))
                self.table2.item(0, 2).setBackground(QColor(200, 0, 10, 150))

        for i in range(3):
            self.table1.item(1, i).setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            self.table2.item(1, i).setTextAlignment(Qt.AlignmentFlag.AlignCenter)


    def update_thres(self, thres):
        pass 
        

class TableModel(QAbstractTableModel):
    def __init__(self, data):
        super().__init__()
        self._data = data

    # def data(self, index, role):
    #     if role == Qt.DisplayRole:
    #         return self._data[index.row()][index.column()]

    #     if role == Qt.BackgroundRole:
    #         value = self._data[index.row()][index.column()]
    #         if(isinstance(value, int) or isinstance(value, float)):
    #             if value > 5:
    #                 return QColor(200, 10, 0, a=150)

    # def rowCount(self, index):
    #     # The length of the outer list.
    #     return len(self._data)

    # def columnCount(self, index):
    #     # The following takes the first sub-list, and returns
    #     # the length (only works if all rows are an equal length)
    #     return len(self._data[0])
    
    # def setData(self, index, data):
    #     self.emit.data 

class Sos_Page(QWidget):
    def __init__(self):
        super().__init__()
        pal = QPalette()
        pal.setColor(QPalette.ColorRole.Window, QColor(255, 0, 0, 230))
        self.setAutoFillBackground(True)
        self.setPalette(pal)
        self.label_word = QLabel("SOS")
        self.label_word.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        label_font = QFont("Anurati", pointSize=100, weight=3)
        self.label_word.setFont(label_font)
        
        self.layout = QVBoxLayout()
        self.layout.addWidget(self.label_word)
        self.setLayout(self.layout)

class Serial_Interface(QObject):
    auth_signal = Signal(int)
    info_signal = Signal(list)
    mode_signal = Signal(str)
    charge_signal = Signal(int)
    sos_signal = Signal(int)
    def __init__(self):
        super().__init__()
        self.port = QSerialPort()
    
    def open(self):
        print("Scanning Port ...")
        port_list = QSerialPortInfo().availablePorts()
        [print(port.portName(), end=" ") for port in port_list]
        com_sel = input("\nPort: COM")
        self.port.setBaudRate(QSerialPort.BaudRate.Baud115200)
        self.port.setPortName("COM"+com_sel)
        self.port.setParity(QSerialPort.Parity.NoParity)
        self.port.setStopBits(QSerialPort.StopBits.OneStop)
        self.port.setDataBits(QSerialPort.DataBits.Data8)
        self.port.setFlowControl(QSerialPort.FlowControl.NoFlowControl)
        self.port.readyRead.connect(self.read)
        while True:
            s = self.port.open(QIODevice.ReadWrite)
            if s:
                print("Port Opened")
                break
            else:
                print("Port Failed to Open", end="")
                com_sel = input("\nPort: COM")
                self.port.setPortName("COM"+com_sel)

    def close(self):
        self.port.close()

    def read(self):
        while(self.port.canReadLine()):
            msg = self.port.readLine()
            decoded_msg = msg.toStdString()
            print(decoded_msg, end='')
            if "12345" in decoded_msg:
                self.auth_signal.emit(1)
                self.write("54321")
            # elif "Mode:" in decoded_msg:
            #     self.mode_signal.emit(decoded_msg)
            elif "Info" in decoded_msg:
                data_str = re.findall(r"\b[+-]?\d+\.\d+\b", decoded_msg)
                data_float = []
                [data_float.append(float(f)) for f in data_str]
                if len(data_float) == 6 or len(data_float) == 4:
                    self.info_signal.emit(data_float)
            elif "Battery" in decoded_msg:
                charge = re.findall(r"\b\d+?[/]", decoded_msg)
                charge_int = int(charge[0][0:-1])
                self.charge_signal.emit(charge_int)
            elif "SOS" in decoded_msg:
                self.sos_signal.emit(2)
            elif "Cleared" in decoded_msg:
                self.sos_signal.emit(1)
            elif "EXPLORATION" in decoded_msg:
                self.mode_signal.emit("Exploration")
            elif "BATTLE" in decoded_msg:
                self.mode_signal.emit("Battle")

    def write(self, data):
        if (self.port.isWritable()):
            self.port.write(data.encode('ascii'))
        print("Wrote:", data)

class main_window(QMainWindow):
    def __init__(self):
        super().__init__()
        self.screen = QStackedWidget()
        self.screen.setSizePolicy(QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Minimum)
        self.login_page = Login_Page()
        self.cyrix_page = Cyrix_Page()
        self.sos_page = Sos_Page()
        self.screen.addWidget(self.login_page)
        self.screen.addWidget(self.cyrix_page)
        self.screen.addWidget(self.sos_page)
        self.screen.setCurrentWidget(self.login_page)
        self.setCentralWidget(self.screen)

        self.ser = Serial_Interface()
        self.ser.auth_signal.connect(self.switch_screen)
        self.ser.sos_signal.connect(self.switch_screen)
        self.ser.info_signal.connect(self.cyrix_page.update_data)
        self.ser.mode_signal.connect(self.cyrix_page.update_mode)
        self.ser.charge_signal.connect(self.cyrix_page.update_charge)
        self.cyrix_page.recharge_signal.connect(self.ser.write)
        self.cyrix_page.cmode_signal.connect(self.ser.write)
        self.setWindowTitle("Authentication")

    
    def switch_screen(self, screen):
        if screen == 0:
            self.screen.setCurrentWidget(self.login_page)
            self.setWindowTitle("Authentication")
        elif screen == 1:
            self.screen.setCurrentWidget(self.cyrix_page)
            self.setWindowTitle("Cyrix Lab")
        elif screen == 2:
            self.screen.setCurrentWidget(self.sos_page)
            self.setWindowTitle("SOS SOS SOS")
        
        

def main():
    app = QApplication(sys.argv)
    m = main_window()
    m.ser.open()
    m.show()
    
    app.exec()

if __name__ == "__main__":
    main()
