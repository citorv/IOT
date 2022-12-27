import requests
from datetime import datetime
import logging
import time

def ultimo_sismo(api_url):
    api_url = "https://api.gael.cloud/general/public/sismos"
    response = requests.get(api_url)
    list = response.json()
    return list[0] #primero de la lista de la api es el mas reciente

if __name__ == "__main__":
    
    #lectura api
    api_url = "https://api.gael.cloud/general/public/sismos"
    sismo = ""
    
    #log a archivo
    log_format = '%(asctime)s|%(levelname)s: %(message)s'
    logging.basicConfig(level=logging.INFO, format=log_format)
    logger = logging.getLogger("sismos")
    f_handler = logging.FileHandler("sismos.log")
    f_handler.setLevel(logging.INFO)
    f_handler.setFormatter(logging.Formatter(log_format))
    logger.addHandler(f_handler)


    while True:
        nuevo_sismo = ultimo_sismo(api_url)
        if sismo == nuevo_sismo:
            time.sleep(60)
        else:
            sismo = nuevo_sismo
            logger.info("nuevo sismo")
            magnitud_sismo = float(sismo["Magnitud"])
            fecha_sismo = sismo["Fecha"]
            refgeo = sismo["RefGeografica"]
            ciudad = refgeo.split(" ")[-1]
            if magnitud_sismo > 6 and ciudad == "Iquique":
                logger.info(f"\nFecha: {fecha_sismo} \nMagnitud: {magnitud_sismo} \nRefGeografica: {refgeo}\nCiudad: {ciudad}\n")
                print("SE ACTIVA SISTEMA DE CIERRE DE VALVULA DE GAS")
            time.sleep(60)

