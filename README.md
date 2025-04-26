# Office Syndrome Prevention System (Project IOT Group7)
## ขั้นตอนการตั้งค่าและใช้งาน
1. ทำการ Clone Project นี้ผ่าน `git clone https://github.com/piyapong-mun/project-iot-group7.git`
2. ทำการ Up docker compose 
   1. เข้า Directory /Docker-Compose `cd Docker-Compose`
   2. Up docker compose `docker compose up`
  
   เพิ่มเติม ใน Dokcer compose จะมี 2 Services หลักคือ :
   - mosquitto -> mqtt server (port 8883)
   - node red -> edge compute (port 1880)
3. Config Node red
   1. เข้า Node red ผ่าน browser `http://localhost:1880`
   2. ตรวจสอบว่าใน Node red Flow อยู่แล้วหรือไม่
      1. หากไม่มี ให้ทำการ import flow โดยจะใช้ flow จาก `/project-iot-group7/Node-Flow-Backup/flow.json`
         - การ import: ไปที่ `Hamburger Icon (แถบสามขีด) > Import > Select a file to import`
      2. Add dashboard ให้กับ node red
         - ไปที่ `Hamburger Icon (แถบสามขีด) > Manage palette > Install`
         - Install `node-red-dashboard`
    3. ตั้งค่า mqtt ให้ node red
       - หา IP ของเครื่องปัจจุบัน
       - แก้ไข ip ให้กับ mqtt `เลือก Node mqtt ใดก็ได้ > ตรง Server กดแก้ไข > ใส่ IP ที่ได้มาลงไป`
    4. ทดสอบ delploy
       - หน้า UI จะอยู่ตรง `http://localhost:1880/ui/` 
4. upload Code 
    1. เข้าไปที่ `project-iot-group7/Arduino-Code/` จะมี สอง folder **M5stack** และ **ESP32** และภายในทั้ง 2 Folder นี้จะเป็น arduino code
    2.  แก้ไข WiFi ssid, password ภายใน file code จากทั้งสอง folder นี้
    3. upload Code ให้กับ ทั้งสอง อุปกรณ์ คือ **M5Stak** และ **ESP32**
    4. รอ M5Stak และ ESP32 เชื่อมต่อกับ mosquitto
