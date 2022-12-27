from flask import Flask
from flask import render_template
from flask import Response
import face_recognition
import cv2
app = Flask(__name__)

image = cv2.imread("data/vic.jpeg")
face_loc = face_recognition.face_locations(image)[0]
face_image_encodings = face_recognition.face_encodings(image, known_face_locations=[face_loc])[0]
cap = cv2.VideoCapture(0)
def generate():
     while True:
    
          ret, frame = cap.read()
          if ret == False: break
          frame = cv2.flip(frame, 1)
          face_location = face_recognition.face_locations(frame)
          if face_location != []:
               for face_location in face_location:
                    face_frame_encodings = face_recognition.face_encodings(frame, known_face_locations=[face_location])[0]
                    result = face_recognition.compare_faces([face_image_encodings    ], face_frame_encodings)
                    #print("Result:", result)
                    if result [0] == True:
                         text = "Victor"
                         color = (125, 220, 0)
                    else:
                         text = "Desconocido"
                         color= (50, 50, 255)
                    cv2.rectangle(frame, (face_location[3], face_location[2]), (face_location[1], face_location[2]+ 30), color, -1)
                    cv2.rectangle(frame, (face_location[3], face_location[0]), (face_location[1], face_location[2]), color, 2)
                    cv2.putText(frame, text, (face_location[3], face_location[2] + 20), 2, 0.7, (255, 255, 255), 1 )
          
          (flag, encodedImage) = cv2.imencode(".jpg", frame)
          if not flag:
               continue
          yield(b'--frame\r\n' b'Content-Type: image/jpeg\r\n\r\n' +
               bytearray(encodedImage) + b'\r\n')             
          
          #cv2.imshow("Frame", frame)
          #k = cv2.waitKey(1)
          #if k == 27 & 0xFF:
               #break
     cap.release() 
     #cv2.destroyAllWindows()     
@app.route("/")
def index():
     return render_template("index.html")
@app.route("/video_feed")
def video_feed():
     return Response(generate(),
          mimetype = "multipart/x-mixed-replace; boundary=frame")
if __name__ == "__main__":
      app.run(host='0.0.0.0', port=8180)
cap.release()