En vez de hacer un sistema operativo con hilos, he implementado una arquitectura basada en Máquinas de Estados. Me parecía la forma más eficiente y ordenada de controlar el flujo del programa.


Para ello he diseñado dos máquinas de estados independientes: la "Principal" (para el servicio de café) y la de "Admin". La máquina principal tiene 9 estados y la de admin 8. El objetivo de esto ha sido desglosar el código en tareas pequeñas y manejables. Por ejemplo, aunque la práctica pedía solo dos estados generales (esperar y servir), yo he dividido el proceso en 7 pasos: espera, detección, mostrar datos, menú, preparación, retirada y reinicio. Al desglosar así el problema, es mucho más fácil controlar qué hace el Arduino en cada momento que pelearse con un bloque de código gigante.


Para la multitarea, he evitado el uso de delay() (salvo para pequeñas estabilizaciones) y lo gestiono todo con un timer "global" usando la función millis(). La técnica que uso se basa en comprobar constantemente si ha pasado el tiempo necesario (millis() - tInicio) o si hay cambios en los sensores (al parecer esta técnica se llama Polling no bloqueante). Esto permite que el bucle loop() dé miles de vueltas por segundo manteniendo el sistema siempre reactivo a los botones, implementando además una lógica de tiempos para diferenciar pulsaciones cortas de largas


Un detalle importante es que he implementado una lógica Anti-Flickering (anti-parpadeo) para la pantalla LCD. En lugar de borrar y escribir la pantalla en cada vuelta del bucle (lo que hace que parpadee y se vea fatal), he añadido una condición para que solo se envíen datos a la pantalla si los valores de temperatura o humedad han cambiado realmente.


Por último, para mejorar la legibilidad, he extraído las lógicas repetitivas a funciones auxiliares, dejando el código principal mucho más limpio.


![Diagrama del circuito](images/p3_bb.png)


[![Ver video](https://img.youtube.com/vi/X1cvMS5Neak/maxresdefault.jpg)](https://youtu.be/X1cvMS5Neak)
