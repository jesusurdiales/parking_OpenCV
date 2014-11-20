//////	DETECCIÓN DE PLAZAS DE APARCAMIENTO	//////

#include "functions.h"

#define camara 1
#define dimensionesROI 15
#define umbralPresencia 100

/* DECLARACIÓN DE FUNCIONES GLOBALES */
void onMouse(int event, int x, int y, int, void*);

/* VARIABLES GLOBALES DEL PROGRAMA */

// Puntos donde se almacenan las coordenadas del pixel en el cual estamos haciendo click
Point pulsar, soltar;
// Plaza donde estará aparcado el coche
Rect plaza;
// Variable que informa al bucle principal que ya hemos seleccionado todas las plazas
bool plazaSeleccionada = false;
int numeroPlazas = 1;

int main()
{
	/* INICIALIZACIÓN DEL PROGRAMA Y ACCESO A LA CÁMARA */

	Mat frame, frame_gray, umbral(frame.size(), CV_8U);
	char esc;

	std::cout << endl << "Inacializacion del programa. Accediendo a la camara...." << endl;
	// Capturamos las imágenes de la cámara del ordenador
	VideoCapture cam(camara);
	// Comprobamos que no existan errores
	if ( !cam.isOpened() )
	{
		std::cout << "Error al acceder a la camara del ordenador." << endl;
		getchar();
		return(EXIT_FAILURE);
	}

	// Ventanas para mostrar los resultados
	namedWindow("camara", 1);
	namedWindow("bordes", 1);
	namedWindow("umbral", 1);
	namedWindow("plazas", 1);

	// Activamos los eventos del ratón
	setMouseCallback("camara", onMouse, 0);

	/* SELECCIÓN DE LAS PLANTAS DE APARCAMIENTO */

	Mat salidaBordes(Size(frame.cols, frame.rows), CV_8U);
	Mat drawing;
	RNG rng(12345);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	cout << endl << "Seleccion de las plazas del aparcamiento." << endl;
	cout << "Pulse la tecla esc si desea cancelar la operacion y salir del programa." << endl << endl;
	while (1)
	{
		cam >> frame;

		// Ha ocurrido un error y no hemos podido acceder al la cámara, salimos
		if (!frame.data)
		{
			cout << "Error capturando frames de la cámara" << endl;
			getchar();
			exit(-1);
		}

		// Eliminación de ruido
		GaussianBlur(frame, frame, Size(3, 3), 0, 0);

		// Realce de bordes
		//frame = realceDeBordes(frame);

		// Umbralizamos el parking para que el algoritmo de detección de bordes funcione mejor
		cvtColor(frame, frame_gray, CV_BGR2GRAY);
		threshold(frame_gray, umbral, 127, 255, CV_THRESH_BINARY_INV);

		// Aplicamos el algoritmo de Canny para la deteccción de bordes
		Canny(frame_gray, salidaBordes, 40, 40*3);
		//dilate(salidaBordes, salidaBordes, Mat(), Point(-1,-1), 3);
		findContours(salidaBordes, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));

		// Buscamos los rectángulos que forman las plazas
		vector<RotatedRect> minRect(contours.size());

		for (unsigned int i = 0; i < contours.size(); i++)
		{
			minRect[i] = minAreaRect(Mat(contours[i]));
		}

		// Dibujamos los contornos y los rectángulos que forman las plazas
		drawing = Mat::zeros(frame.size(), CV_8UC3);
		for (unsigned int i = 0; i < contours.size(); i++)
		{
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			// contour
			drawContours(drawing, contours, i, color, 1, 8, hierarchy, 0, Point());
			// rotated rectangle
			Point2f rect_points[4]; minRect[i].points(rect_points);
			for (int j = 0; j < 4; j++)
				line(drawing, rect_points[j], rect_points[(j + 1) % 4], color, 1, 8);
		}

		// Mostramos los resultados durante la elección de la plaza
		imshow("camara", frame);
		imshow("umbral", umbral);
		imshow("bordes", salidaBordes);
		imshow("plazas", drawing);

		// Podemos salir del bucle pulsando cualquier tecla manualmente o cuando haya finalizado la selección de plazas sale 
		// automáticamente. Para salir del programa hay que pulsar la tecla esc
		esc = cv::waitKey(30);
		if ( (plazaSeleccionada == true) )
			break;
		if (esc == 27)
			return 0;
	}
	// Una vez seleccionada la plaza procedemos a eliminar las ventanas anteriormente abiertas
	destroyAllWindows();

	/* CREACIÓN DE LAS REGIONES DE INTERÉS (ROIs) */
	Rect ROI1(0, 0, 0, 0), ROI2(0, 0, 0, 0);

	// Antes de nada es necesario comprobar si la plaza está en horizontal o vertical respecto a la cámara
	if (plaza.width <= plaza.height)
	{
		// Si el rectángulo tiene más de 10x20 píxeles podemos crear dentro dos rectángulos de 15x15
		if (plaza.width > 20 && plaza.height > 40)
		{
			int x0, x2, y2, y0;
			// Calculamos las posiciones de las ROIs y sus dimensiones
			x2 = plaza.x + plaza.width;
			y2 = plaza.y + plaza.height;
			x0 = cvRound(plaza.x + (x2 - plaza.x) / 2 - (dimensionesROI / 2));
			y0 = cvRound(plaza.y + (y2 - plaza.y) / 4 - (dimensionesROI / 2));
			// Calculamos ROI1 y ROI2
			ROI1 = Rect(x0, y0, dimensionesROI, dimensionesROI);
			y0 = cvRound(plaza.y + 3 * (y2 - plaza.y) / 4 - (dimensionesROI / 2));
			ROI2 = Rect(x0, y0, dimensionesROI, dimensionesROI);
		}
		else
		{
			// Si el tamaño de la plaza es demasiado pequeño, el algoritmo no va a funcionar bien, por lo que se desecha
			if ((plaza.width < dimensionesROI) || (plaza.height < dimensionesROI))
			{
				cout << "La plaza seleccionada es demasiado pequeña para el correcto funcionamiento del algoritmo." << endl;
				cout << "Fin del programa" << endl;
				getchar();
				exit(EXIT_FAILURE);
			}
			// Solo tendríamos en este caso una ROI en el centro de la plaza
			int x0, y0;
			x0 = cvRound(plaza.x + plaza.width / 2 - dimensionesROI / 2);
			y0 = cvRound(plaza.y + plaza.height / 2 - dimensionesROI / 2);
			ROI1 = Rect(x0, y0, dimensionesROI, dimensionesROI);
		}
	}
	else
	// La plaza está horizontal respesto a la cámara. El código será igual que el anterior, pero modificando la anchura por altura
	{
		{
			// Si el rectángulo tiene más de 10x20 píxeles podemos crear dentro dos rectángulos de 15x15
			if (plaza.height > 20 && plaza.width > 40)
			{
				int x0, x2, y2, y0;
				// Calculamos las posiciones de las ROIs y sus dimensiones
				x2 = plaza.x + plaza.width;
				y2 = plaza.y + plaza.height;
				x0 = cvRound(plaza.x + (x2 - plaza.x) / 4 - (dimensionesROI / 2));
				y0 = cvRound(plaza.y + (y2 - plaza.y) / 2 - (dimensionesROI / 2));
				// Calculamos ROI1 y ROI2
				ROI1 = Rect(x0, y0, dimensionesROI, dimensionesROI);
				x0 = cvRound(plaza.x + 3 * (x2 - plaza.x) / 4 - (dimensionesROI / 2));
				ROI2 = Rect(x0, y0, dimensionesROI, dimensionesROI);
			}
			else
			{
				// Si el tamaño de la plaza es demasiado pequeño, el algoritmo no va a funcionar bien, por lo que se desecha
				if ((plaza.width < dimensionesROI) || (plaza.height < dimensionesROI))
				{
					cout << "La plaza seleccionada es demasiado pequeña para el correcto funcionamiento del algoritmo." << endl;
					cout << "Fin del programa" << endl;
					getchar();
					exit(EXIT_FAILURE);
				}
				// Solo tendríamos en este caso una ROI en el centro de la plaza
				int x0, y0;
				x0 = cvRound(plaza.x + plaza.width / 2 - dimensionesROI / 2);
				y0 = cvRound(plaza.y + plaza.height / 2 - dimensionesROI / 2);
				ROI1 = Rect(x0, y0, dimensionesROI, dimensionesROI);
			}
		}
	}

	// Guardamos las imágenes de los ROIs cuando el parking está vacío
	Mat imagen_ROI1_libre(frame_gray, ROI1);
	medianBlur(imagen_ROI1_libre, imagen_ROI1_libre, 3);
	Mat imagen_ROI2_libre(frame_gray, ROI2);
	medianBlur(imagen_ROI2_libre, imagen_ROI2_libre, 3);
	imshow("ROI1", imagen_ROI1_libre);
	imshow("ROI2", imagen_ROI2_libre);
	
	/* VIGILANCIA DEL APARCAMIENTO */

	Mat imagen_ROI1, imagen_ROI2, frame_vigilancia, frame_vigilancia_gray;
	double norm1, norm2;
	bool plazaOcupada;
	Scalar rojo = Scalar(0, 0, 255);
	Scalar verde = Scalar(0, 255, 0);
	namedWindow("Vigilancia", 1);

	cout << endl << "Plazas seleccionadas, procediendo a la vigilancia del aparcamiento..." << endl << endl;
	while (1)
	{
		cam >> frame_vigilancia;

		// Si ha ocurrido un error y no hemos podido acceder al la cámara, salimos
		if (!frame_vigilancia.data)
		{
			cout << "Error capturando frames de la cámara" << endl;
			getchar();
			exit(EXIT_FAILURE);
		}

		// Eliminación de ruido y conversión a escala de grises
		GaussianBlur(frame_vigilancia, frame_vigilancia, Size(3, 3), 0, 0);
		cvtColor(frame_vigilancia, frame_vigilancia_gray, CV_BGR2GRAY);

		// Para cada ROI actualizamos las imágenes y calculamos la "norma relativa diferencial"
		imagen_ROI1 = Mat(frame_vigilancia_gray, ROI1);
		medianBlur(imagen_ROI1, imagen_ROI1, 3);
		imagen_ROI2 = Mat(frame_vigilancia_gray, ROI2);
		medianBlur(imagen_ROI2, imagen_ROI2, 3);

		norm1 = norm(imagen_ROI1_libre, imagen_ROI1, NORM_L2);
		cout << "Norma 1: " << norm1 << endl;
		norm2 = norm(imagen_ROI2_libre, imagen_ROI2, NORM_L2);
		cout << "Norma 2: " << norm2 << endl;

		// Si una de estas normas supera el umbral, asumimos la presencia de un coche
		if ((norm1 > umbralPresencia) || (norm2 > umbralPresencia))
		{
			cout << "Plaza ocupada" << endl;
			plazaOcupada = true;
		}
		else
		{
			cout << "Plaza libre" << endl;
			plazaOcupada = false;
		}

		// Dibujamos las ROIs y la plaza
		if (norm1 > umbralPresencia)
			rectangle(frame_vigilancia, ROI1, rojo);
		else
			rectangle(frame_vigilancia, ROI1, verde);
		// Dependiendo del tamaño del aparcamiento puede ser que no haya ROI2
		if (ROI2.area() > 0)
		{
			if (norm2 > umbralPresencia)
				rectangle(frame_vigilancia, ROI2, rojo);
			else
				rectangle(frame_vigilancia, ROI2, verde);
		}

		if (plazaOcupada == true)
			rectangle(frame_vigilancia, plaza, rojo);
		else
			rectangle(frame_vigilancia, plaza, verde);

		// Mostramos la imagen del aparcamiento captada por la cámara
		imshow("Vigilancia", frame_vigilancia);

		// Actualizamos el frame cada 1 segundo. Salimos del programa pulsando la tecla esc
		esc = waitKey(30);
		if (esc == 27)
			break;
	}
	
	destroyAllWindows();
	cout << "\nHa seleccionado salir del programa." << endl;
	cout << "Pulse una tecla para continuar." << endl;
	getchar();
	return 0;
}

//////	FUNCIONES GLOBALES	//////

void onMouse(int event, int x, int y, int, void*)
{
	// Vigilando cuando pulsas el ratón (inicio del rectángulo) y cuando lo sueltas (final del mismo) podemos extraer el rectángulo
	// que contiene el objeto a rastrear
	switch (event)
	{
	// Evento pulsar boton del ratón
	case CV_EVENT_LBUTTONDOWN:
		pulsar = Point(x, y);
		break;
	// Evento soltar botón del ratón
	case CV_EVENT_LBUTTONUP:
		soltar = Point(x, y);
		// Construimos la plaza de aparcamiento
		plaza.x = MIN(pulsar.x, soltar.x);
		plaza.y = MIN(pulsar.y, soltar.y);
		plaza.width = std::abs(pulsar.x - soltar.x);
		plaza.height = std::abs(pulsar.y - soltar.y);
		plazaSeleccionada = true;
		break;
	default:
		break;
	}
}