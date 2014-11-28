//////	DETECCI�N DE PLAZAS DE APARCAMIENTO	//////

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\ml\ml.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\opencv.hpp>
#include <opencv2\video\tracking.hpp>
#include <iostream>
#include <vector>
#include <Windows.h>
#include <string>
#include <sstream>

using namespace std;
using namespace cv;

#define camara 1
#define dimensionesROI 30
#define umbralPresencia 200

/* DECLARACI�N DE FUNCIONES GLOBALES */
void onMouse(int event, int x, int y, int, void*);
int calcular_umbral(Mat plaza);

/* VARIABLES GLOBALES DEL PROGRAMA */

// Puntos donde se almacenan las coordenadas del pixel en el cual estamos haciendo click
Point pulsar, soltar;
// Plaza donde estar� aparcado el coche
vector<Rect> plaza;
Rect seleccion;
Mat frame;
bool plazaSeleccionada = false;
int numeroPlazas = 1;
int plazasSeleccionadas = 0;
int umbral_dinamico = 0;

int main()
{
	/* INICIALIZACI�N DEL PROGRAMA Y ACCESO A LA C�MARA */

	Mat frame_gray;
	char esc;

	std::cout << endl << "Inacializacion del programa. Accediendo a la camara...." << endl;
	// Capturamos las im�genes de la c�mara del ordenador
	VideoCapture cam(camara);
	// Comprobamos que no existan errores
	if ( !cam.isOpened() )
	{
		std::cout << "Error al acceder a la camara del ordenador." << endl;
		system("pause");
		return(EXIT_FAILURE);
	}

	// Selecci�n del n�mero de plazas
	namedWindow("PlazasDisponibles", 1);
	int count = 0;
	while (count < 15)
	{
		cam >> frame;
		waitKey(30);
		imshow("PlazasDisponibles", frame);
		count++;
	}
	cout << endl << "Introduzca el numero de plazas: ";
	cin >> numeroPlazas;
	while (numeroPlazas <= 0 || numeroPlazas > 12)
	{
		cout << "Seleccione un numero de plazas entre 1 y 12." << endl;
		cout << endl << "Introduzca el numero de plazas: ";
		cin >> numeroPlazas;
	}
	destroyWindow("PlazasDisponibles");
	plaza.resize(numeroPlazas);

	// Ventanas para mostrar los resultados
	namedWindow("Camara", 1);

	// Activamos los eventos del rat�n sobre la ventana "camara"
	setMouseCallback("Camara", onMouse, 0);

	/* SELECCI�N DE LAS PLANTAS DE APARCAMIENTO */

	cout << endl << "Seleccion de las plazas del aparcamiento." << endl;
	cout << "Pulse la tecla esc si desea cancelar la operacion y salir del programa." << endl << endl;

	while (1)
	{
		cam >> frame;

		// Ha ocurrido un error y no hemos podido acceder al la c�mara, salimos
		if (!frame.data)
		{
			cout << "Error capturando frames de la c�mara. Fin del programa." << endl;
			system("pause");
			exit(-1);
		}

		// Eliminaci�n de ruido
		GaussianBlur(frame, frame, Size(3, 3), 0, 0);

		// Mostramos los resultados durante la elecci�n de la plaza
		imshow("Camara", frame);

		// Podemos salir del bucle pulsando cualquier tecla manualmente o cuando haya finalizado la selecci�n de plazas sale 
		// autom�ticamente. Para salir del programa hay que pulsar la tecla esc
		esc = cv::waitKey(30);
		if ( numeroPlazas == plazasSeleccionadas )
			break;
		if (esc == 27)
			return 0;
	}
	// Una vez seleccionada la plaza procedemos a eliminar las ventanas anteriormente abiertas
	destroyAllWindows();

	/* CREACI�N DE LAS REGIONES DE INTER�S (ROIs) */
	vector<Rect> ROIs1;
	vector<Rect> ROIs2;

	// Antes de nada es necesario comprobar si cada plaza est� en horizontal o vertical respecto a la c�mara
	for (int i = 0; i < numeroPlazas; i++)
	{
		// C�lculo del umbral adecuado teniendo en cuenta todas las plazas
		cvtColor(frame, frame_gray, CV_BGR2GRAY);
		umbral_dinamico = calcular_umbral(Mat(frame_gray, plaza.at(i)));

		if (plaza[i].width <= plaza[i].height)
		{
			// Si el rect�ngulo tiene m�s de 10x20 p�xeles podemos crear dentro dos rect�ngulos de 15x15
			if (plaza[i].width > (dimensionesROI + 10) && plaza[i].height > (dimensionesROI * 2 + 10))
			{
				int x0, x2, y2, y0;
				// Calculamos las posiciones de las ROIs y sus dimensiones
				x2 = plaza[i].x + plaza[i].width;
				y2 = plaza[i].y + plaza[i].height;
				x0 = cvRound(plaza[i].x + (x2 - plaza[i].x) / 2 - (dimensionesROI / 2));
				y0 = cvRound(plaza[i].y + (y2 - plaza[i].y) / 4 - (dimensionesROI / 2));
				// Calculamos ROI1 y ROI2
				ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
				y0 = cvRound(plaza[i].y + 3 * (y2 - plaza[i].y) / 4 - (dimensionesROI / 2));
				ROIs2.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
			}
			else
			{
				// Si el tama�o de la plaza es demasiado peque�o, el algoritmo no va a funcionar bien, por lo que se desecha
				if ((plaza[i].width < dimensionesROI) || (plaza[i].height < dimensionesROI))
				{
					cout << "La plaza seleccionada es demasiado peque�a para el correcto funcionamiento del algoritmo." << endl;
					cout << "Fin del programa" << endl;
					system("pause");
					exit(EXIT_FAILURE);
				}
				// Solo tendr�amos en este caso una ROI en el centro de la plaza
				int x0, y0;
				x0 = cvRound(plaza[i].x + plaza[i].width / 2 - dimensionesROI / 2);
				y0 = cvRound(plaza[i].y + plaza[i].height / 2 - dimensionesROI / 2);
				ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
			}
		}
		else
			// La plaza est� horizontal respesto a la c�mara. El c�digo ser� igual que el anterior, pero modificando la anchura por altura
		{
			{
				// Si el rect�ngulo tiene m�s de 10x20 p�xeles podemos crear dentro dos rect�ngulos de 15x15
				if (plaza[i].height > (dimensionesROI + 10) && plaza[i].width > (dimensionesROI * 2 + 10))
				{
					int x0, x2, y2, y0;
					// Calculamos las posiciones de las ROIs y sus dimensiones
					x2 = plaza[i].x + plaza[i].width;
					y2 = plaza[i].y + plaza[i].height;
					x0 = cvRound(plaza[i].x + (x2 - plaza[i].x) / 4 - (dimensionesROI / 2));
					y0 = cvRound(plaza[i].y + (y2 - plaza[i].y) / 2 - (dimensionesROI / 2));
					// Calculamos ROI1 y ROI2
					ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
					x0 = cvRound(plaza[i].x + 3 * (x2 - plaza[i].x) / 4 - (dimensionesROI / 2));
					ROIs2.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
				}
				else
				{
					// Si el tama�o de la plaza es demasiado peque�o, el algoritmo no va a funcionar bien, por lo que se desecha
					if ((plaza[i].width < dimensionesROI) || (plaza[i].height < dimensionesROI))
					{
						cout << "La plaza seleccionada es demasiado peque�a para el correcto funcionamiento del algoritmo." << endl;
						cout << "Fin del programa" << endl;
						system("pause");
						exit(EXIT_FAILURE);
					}
					// Solo tendr�amos en este caso una ROI en el centro de la plaza
					int x0, y0;
					x0 = cvRound(plaza[i].x + plaza[i].width / 2 - dimensionesROI / 2);
					y0 = cvRound(plaza[i].y + plaza[i].height / 2 - dimensionesROI / 2);
					ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
				}
			}
		}
	}

	// Guardamos las im�genes de los ROIs cuando el parking est� vac�o
	vector<Mat> fondo_ROI1;
	vector<Mat> fondo_ROI2;
	threshold(frame_gray, frame_gray, umbral_dinamico, 255, CV_THRESH_BINARY);

	for (int i = 0; i < numeroPlazas; i++)
	{
		fondo_ROI1.push_back(Mat(frame_gray, ROIs1[i]));
		medianBlur(fondo_ROI1[i], fondo_ROI1[i], 3);
		fondo_ROI2.push_back(Mat(frame_gray, ROIs2[i]));
		medianBlur(fondo_ROI2[i], fondo_ROI2[i], 3);
	}
	
	/* VIGILANCIA DEL APARCAMIENTO */

	vector<Mat> imagen_ROI1, imagen_ROI2;
	Mat frame_vigilancia, frame_vigilancia_gray, frame_vigilancia_binario, salida_canny, frame_vigilancia_umbral;
	double norm1, norm2;
	bool plazaOcupada = false;
	Scalar rojo = Scalar(0, 0, 255);
	Scalar verde = Scalar(0, 255, 0);

	int numeroPlazasOcupadas = 0, numeroPlazasLibres = numeroPlazas;
	stringstream stream;
	stream << numeroPlazas;
	String plazasTotales = "Plazas totales: " + stream.str();
	String plazasLibres = "";
	String plazasOcupadas = "";
	stream.str("");
	namedWindow("Vigilancia", 1);

	cout << endl << "Plazas seleccionadas, procediendo a la vigilancia del aparcamiento..." << endl << endl;

	while (1)
	{
		cam >> frame_vigilancia;

		// Si ha ocurrido un error y no hemos podido acceder al la c�mara, salimos
		if (!frame_vigilancia.data)
		{
			cout << "Error capturando frames de la c�mara" << endl;
			system("pause");
			exit(EXIT_FAILURE);
		}

		// Eliminaci�n de ruido y conversi�n a escala de grises
		GaussianBlur(frame_vigilancia, frame_vigilancia, Size(3, 3), 0, 0);
		cvtColor(frame_vigilancia, frame_vigilancia_gray, CV_BGR2GRAY);

		// Umbralizaci�n de la imagen: gracias a que el umbral se actualiza constantemente en caso de que no haya coche, 
		// podemos ser inmunes a los cambuos de iluminaci�n
		threshold(frame_vigilancia_gray, frame_vigilancia_umbral, umbral_dinamico, 255, CV_THRESH_BINARY);
		// Usando la detecci�n de bordes de Canny podemos conseguir a�adir p�xeles blancos a nuestra imagen provenientes de los 
		// bordes de los coches oscuros, que la umbralizaci�n no detecta
		Canny(frame_vigilancia_gray, salida_canny, 40, 2 * 40);

		// Unimos las im�genes extraidas de Canny y de la umbralizaci�n, para obtener una imagen binaria con los pixeles blancos de ambas
		bitwise_or(frame_vigilancia_umbral, salida_canny, frame_vigilancia_binario);

		// Calculamos para cada plaza si est� libre u ocupada
		for (int i = 0; i < numeroPlazas; i++)
		{
			// Para cada ROI actualizamos las im�genes y calculamos la "norma relativa diferencial"
			imagen_ROI1.push_back(Mat(frame_vigilancia_binario, ROIs1[i]));
			imagen_ROI2.push_back(Mat(frame_vigilancia_binario, ROIs2[i]));

			norm1 = norm(fondo_ROI1[i], imagen_ROI1[i], NORM_L2);
			norm2 = norm(fondo_ROI2[i], imagen_ROI2[i], NORM_L2);

			// Si una de estas normas supera el umbral, asumimos la presencia de un coche
			if ((norm1 > umbralPresencia) || (norm2 > umbralPresencia))
			{
				plazaOcupada = true;
				numeroPlazasOcupadas++;
			}
			else
			{
				plazaOcupada = false;
				numeroPlazasLibres++;
				// Calculamos el umbral a partir de los ROIs
				umbral_dinamico = calcular_umbral(Mat(frame_vigilancia_gray, plaza[i]));
			}

			// Dibujamos las ROIs y la plaza
			if (norm1 > umbralPresencia)
				rectangle(frame_vigilancia, ROIs1[i], rojo);
			else
				rectangle(frame_vigilancia, ROIs1[i], verde);
			// Dependiendo del tama�o del aparcamiento puede ser que no haya ROI2
			if (ROIs2[i].area() > 0)
			{
				if (norm2 > umbralPresencia)
					rectangle(frame_vigilancia, ROIs2[i], rojo);
				else
					rectangle(frame_vigilancia, ROIs2[i], verde);
			}

			if (plazaOcupada == true)
				rectangle(frame_vigilancia, plaza[i], rojo);
			else
				rectangle(frame_vigilancia, plaza[i], verde);
		}

		// Mostramos por pantalla la cantidad de plazas, las plazas libres y las plazas ocupadas
		putText(frame_vigilancia, plazasTotales, Point(10, 15), FONT_HERSHEY_COMPLEX, 0.5, Scalar(0, 255, 0));
		stream.str("");
		stream << numeroPlazasOcupadas;
		plazasOcupadas = "Plazas ocupadas: " + stream.str();
		putText(frame_vigilancia, plazasOcupadas, Point(10, frame_vigilancia.rows - 10), FONT_HERSHEY_COMPLEX, 0.5, Scalar(0, 0, 255));
		stream.str("");
		stream << numeroPlazasLibres;
		plazasLibres = "Plazas libres: " + stream.str();
		putText(frame_vigilancia, plazasLibres, Point(10, frame_vigilancia.rows - 30), FONT_HERSHEY_COMPLEX, 0.5, Scalar(0, 255, 0));

		// Mostramos la imagen del aparcamiento captada por la c�mara
		imshow("Vigilancia", frame_vigilancia);
		//imshow("Umbral", frame_vigilancia_binario);
		//imshow("Canny", salida_canny);

		numeroPlazasLibres = numeroPlazasOcupadas = 0;
		// Actualizamos el frame cada 1 segundo. Salimos del programa pulsando la tecla esc
		esc = waitKey(30);
		if (esc == 27)
			break;
	}
	
	destroyAllWindows();
	cout << "\nHa seleccionado salir del programa." << endl;
	system("pause");
	return 0;
}

//////	FUNCIONES GLOBALES	//////

void onMouse(int event, int x, int y, int, void*)
{
	// Vigilando cuando pulsas el rat�n (inicio del rect�ngulo) y cuando lo sueltas (final del mismo) podemos extraer el rect�ngulo
	// que contiene el objeto a rastrear
	switch (event)
	{
	// Evento pulsar boton del rat�n
	case CV_EVENT_LBUTTONDOWN:
		pulsar = Point(x, y);
		break;

	// Evento soltar bot�n del rat�n
	case CV_EVENT_LBUTTONUP:
		soltar = Point(x, y);
		// Construimos la plaza de aparcamiento
		plaza[plazasSeleccionadas].x = MIN(pulsar.x, soltar.x);
		plaza[plazasSeleccionadas].y = MIN(pulsar.y, soltar.y);
		plaza[plazasSeleccionadas].width = std::abs(pulsar.x - soltar.x);
		plaza[plazasSeleccionadas].height = std::abs(pulsar.y - soltar.y);
		plazaSeleccionada = true;
		plazasSeleccionadas++;
		break;
	default:
		break;
	}
}

int calcular_umbral(Mat plaza)
{
	int max_blanco = 0;
	int nuevo_umbral = 0;

	// Recorremos la imagen en busca del pixel m�s claro distinto de blanco, a partir del cual definimos el nuevo umbral
	for (int i = 0; i < plaza.cols; i++)
	{
		uchar *data = plaza.ptr<uchar>(i);

		for (int j = 0; j < plaza.rows; j++)
		{
			if (data[j] < 220 && data[j] > max_blanco)
				max_blanco = data[j];
		}
	}

	// Calculamos el nuevo umbral y lo comparamos con el existente
	nuevo_umbral = max_blanco + 10;

	// Si el cambio no es significativo, no actualizamos el umbral
	if (abs(umbral_dinamico - nuevo_umbral) < 10)
		return umbral_dinamico;
	else
		return nuevo_umbral;
}