//////	DETECCI�N DE COCHES EN PLAZAS DE APARCAMIENTO	//////

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

// Puntos donde se almacenan las coordenadas del pixel en el cual estamos haciendo click izquierdo
Point pulsar, soltar;
// Vector de plazas donde estar�n aparcados los coches
vector<Rect> plaza;
int numeroPlazas = 0;
int umbral_dinamico = 0;

int main()
{
	/* INICIALIZACI�N DEL PROGRAMA Y ACCESO A LA C�MARA */

	// Imagen capturada por la c�mara
	Mat frame;
	Mat frame_gray;
	char tecla = 32;	// Tecla espacio
	Scalar naranja = Scalar(0, 127, 255);

	std::cout << endl << "Inicializacion del programa. Accediendo a la camara...." << endl;
	// Capturamos las im�genes de la c�mara del ordenador
	VideoCapture cam(camara);
	// Comprobamos que no existan errores
	if ( !cam.isOpened() )
	{
		std::cout << "Error al acceder a la camara del ordenador." << endl;
		system("pause");
		exit(EXIT_FAILURE);
	}

	// Ventanas para mostrar los resultados
	namedWindow("SelecionPlazas", 1);

	// Activamos los eventos del rat�n sobre la ventana "SelecionPlazas"
	setMouseCallback("SelecionPlazas", onMouse, 0);

	/* SELECCI�N DE LAS PLAZAS DE APARCAMIENTO */

	String stringPlazasSelecionadas;
	stringstream numero;

	cout << endl << "Seleccion de las plazas del aparcamiento." << endl;
	cout << "Pulse la tecla 'esc' si desea cancelar la operacion y salir del programa." << endl;
	cout << "Cuando todas las plazas hayan sido seleccionadas pulse la tecla ENTER." << endl << endl;

	while (1)
	{
		cam >> frame;

		// Si ha ocurrido un error y no hemos podido acceder al la c�mara, salimos del programa
		if (!frame.data)
		{
			cout << "Error capturando frames de la c�mara. Fin del programa." << endl;
			system("pause");
			exit(EXIT_FAILURE);
		}

		// Eliminaci�n de ruido
		GaussianBlur(frame, frame, Size(3, 3), 0, 0);
		
		// Mostramos las plazas por el momento seleccionadas y el n�mero de estas
		for (unsigned int i = 0; i < plaza.size(); i++)
		{
			rectangle(frame, plaza[i], naranja, 2);
		}
		numero.str("");
		numero << numeroPlazas;
		stringPlazasSelecionadas = "Plazas seleccionadas: " + numero.str();
		putText(frame, stringPlazasSelecionadas, Point(10, 15), FONT_HERSHEY_COMPLEX, 0.5, naranja);

		// Mostramos la imagen del parking durante la elecci�n de las plazas
		imshow("SelecionPlazas", frame);

		// Para salir del programa es necesario pulsar la tecla esc
		// Para confirmar la selecci�n de plazas ENTER
		tecla = cv::waitKey(30);
		if ( tecla == 13)
			break;
		if (tecla == 27)
			return 0;
	}
	// Una vez seleccionadas las plazas procedemos a eliminar las ventanas anteriormente abiertas
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

		// Plaza vertical
		if (plaza[i].width <= plaza[i].height)
		{
			// Si la plaza es m�s grande que el tama�o de dos ROIs podemos crear dentro dos rect�ngulos de 30 x 30
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
				// Solo tendr�amos en este caso una ROI en el centro de la plaza
				int x0, y0;
				x0 = cvRound(plaza[i].x + plaza[i].width / 2 - dimensionesROI / 2);
				y0 = cvRound(plaza[i].y + plaza[i].height / 2 - dimensionesROI / 2);
				ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
				// Rellenamos el vector de ROIs2 con rect�ngulos de �rea 0 para evitar problemas m�s adelente
				ROIs2.push_back(Rect(0, 0, 0, 0));
			}
		}
		else
		// La plaza est� horizontal respesto a la c�mara. El c�digo ser� igual que el anterior, pero modificando la anchura por altura
		{
			{
				// Si la plaza es m�s grande que el tama�o de dos ROIs podemos crear dentro dos rect�ngulos de 30 x 30
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
					// Solo tendr�amos en este caso una ROI en el centro de la plaza
					int x0, y0;
					x0 = cvRound(plaza[i].x + plaza[i].width / 2 - dimensionesROI / 2);
					y0 = cvRound(plaza[i].y + plaza[i].height / 2 - dimensionesROI / 2);
					ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
					// Rellenamos el vector de ROIs2 con rect�ngulos de �rea 0 para evitar problemas m�s adelente
					ROIs2.push_back(Rect(0, 0, 0, 0));
				}
			}
		}
	}

	// Guardamos las im�genes bajo los ROIs cuando el parking est� vac�o
	vector<Mat> fondo_ROI1;
	vector<Mat> fondo_ROI2;
	threshold(frame_gray, frame_gray, umbral_dinamico, 255, CV_THRESH_BINARY);

	for (int i = 0; i < numeroPlazas; i++)
	{
		fondo_ROI1.push_back(Mat(frame_gray, ROIs1[i]));
		// Aplicamos un filtro de madiana para eliminar el ruido impulsional, el cual afectar�a mucho a la calidad de los resultados,
		// pues estamos manejando im�genes binarias
		medianBlur(fondo_ROI1[i], fondo_ROI1[i], 3);
		fondo_ROI2.push_back(Mat(frame_gray, ROIs2[i]));
		medianBlur(fondo_ROI2[i], fondo_ROI2[i], 3);
	}
	
	/* VIGILANCIA DEL APARCAMIENTO */

	// Im�genes bajo los ROIs capturadas por la c�mara
	vector<Mat> imagen_ROI1, imagen_ROI2;
	imagen_ROI1.resize(numeroPlazas);
	imagen_ROI2.resize(numeroPlazas);
	Mat frame_vigilancia_binario, salida_canny, frame_vigilancia_umbral;
	double norm1 = 0.0, norm2 = 0.0;
	bool plazaOcupada = false;
	Scalar rojo = Scalar(0, 0, 255);
	Scalar verde = Scalar(0, 255, 0);

	// Variables relacionadas con el texto que se muestra por pantalla
	int numeroPlazasOcupadas = 0, numeroPlazasLibres = numeroPlazas;
	stringstream stream;
	stream << numeroPlazas;
	String plazasTotales = "Plazas totales: " + stream.str();
	String plazasLibres = "";
	String plazasOcupadas = "";
	Rect recuadroTexto(5, frame.rows - 45, 200, 42);
	stream.str("");

	// Ventana donde mostramos el proceso de vigilancia del parking
	namedWindow("Vigilancia", 1);

	cout << endl << endl << "Plazas seleccionadas, procediendo a la vigilancia del aparcamiento..." << endl;
	cout << "Puede salir del programa en cualquier momento pulsando la tecla 'esc'." << endl;

	while (1)
	{
		cam >> frame;

		// Si ha ocurrido un error y no hemos podido acceder al la c�mara, salimos
		if (!frame.data)
		{
			cout << "Error capturando frames de la c�mara" << endl;
			system("pause");
			exit(EXIT_FAILURE);
		}

		// Eliminaci�n de ruido y conversi�n a escala de grises
		GaussianBlur(frame, frame, Size(3, 3), 0, 0);
		cvtColor(frame, frame_gray, CV_BGR2GRAY);

		// Umbralizaci�n de la imagen: gracias a que el umbral se actualiza constantemente en caso de que no haya coche, 
		// podemos ser inmunes a los cambios de iluminaci�n
		threshold(frame_gray, frame_vigilancia_umbral, umbral_dinamico, 255, CV_THRESH_BINARY);
		// Usando la detecci�n de bordes de Canny podemos conseguir a�adir p�xeles blancos a nuestra imagen provenientes de los 
		// bordes de los coches oscuros, que la umbralizaci�n no detecta
		Canny(frame_gray, salida_canny, 40, 2 * 40);

		// Unimos las im�genes extraidas de Canny y de la umbralizaci�n, para obtener una imagen binaria con los pixeles blancos de ambas
		bitwise_or(frame_vigilancia_umbral, salida_canny, frame_vigilancia_binario);

		// Calculamos para cada plaza si est� libre u ocupada
		for (int i = 0; i < numeroPlazas; i++)
		{
			// Para cada ROI actualizamos las im�genes y calculamos la norma entre estas y las guardas anteriormente cuando el
			// parking estaba vac�o
			imagen_ROI1.at(i) = (Mat(frame_vigilancia_binario, ROIs1[i]));
			imagen_ROI2.at(i) = (Mat(frame_vigilancia_binario, ROIs2[i]));

			// C�lculo de la norma entre la imagen del ROI con el parking vac�o y la actual capturada con la c�mara
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
				umbral_dinamico = calcular_umbral(Mat(frame_gray, plaza[i]));
			}

			// Dibujamos las ROIs sobre la imagen
			if (norm1 > umbralPresencia)
				rectangle(frame, ROIs1[i], rojo);
			else
				rectangle(frame, ROIs1[i], verde);
			// Dependiendo del tama�o del aparcamiento puede ser que no haya ROI2
			if (ROIs2[i].area() > 0)
			{
				if (norm2 > umbralPresencia)
					rectangle(frame, ROIs2[i], rojo);
				else
					rectangle(frame, ROIs2[i], verde);
			}

			// Dibujamos la plaza
			if (plazaOcupada == true)
				rectangle(frame, plaza[i], rojo, 2);
			else
				rectangle(frame, plaza[i], verde, 2);
		}

		// Mostramos por pantalla la cantidad de plazas, las plazas libres y las plazas ocupadas
		putText(frame, plazasTotales, Point(10, 15), FONT_HERSHEY_COMPLEX, 0.5, naranja);
		stream.str("");
		stream << numeroPlazasOcupadas;
		plazasOcupadas = "Plazas ocupadas: " + stream.str();
		putText(frame, plazasOcupadas, Point(10, frame.rows - 10), FONT_HERSHEY_COMPLEX, 0.5, rojo);
		stream.str("");
		stream << numeroPlazasLibres;
		plazasLibres = "Plazas libres: " + stream.str();
		putText(frame, plazasLibres, Point(10, frame.rows - 30), FONT_HERSHEY_COMPLEX, 0.5, verde);
		rectangle(frame, recuadroTexto, verde);

		// Mostramos la imagen del aparcamiento captada por la c�mara
		imshow("Vigilancia", frame);
		//imshow("UmbralyCanny", frame_vigilancia_binario);
		//imshow("Canny", salida_canny);

		numeroPlazasLibres = numeroPlazasOcupadas = 0;
		// Actualizamos el frame cada 30 milisegundos. Salimos del programa pulsando la tecla esc
		tecla = waitKey(30);
		if (tecla == 27)
			break;
	}
	
	destroyAllWindows();
	cout << "\nHa seleccionado salir del programa." << endl;
	// Esperamos a pulsar una tecla
	system("pause");
	return 0;
}

//////	FUNCIONES GLOBALES	//////

/* Funci�n que maneja los eventos del rat�n */
void onMouse(int event, int x, int y, int, void*)
{
	// Vigilando cuando pulsas el rat�n y cuando lo sueltas podemos extraer el rect�ngulo que contiene la plaza
	switch (event)
	{
	// Evento pulsar boton izquierdo del rat�n
	case CV_EVENT_LBUTTONDOWN:
		pulsar = Point(x, y);
		break;

	// Evento soltar bot�n izquierdo del rat�n
	case CV_EVENT_LBUTTONUP:
	{
		soltar = Point(x, y);
		// Construimos la plaza de aparcamiento a partir de las dos coordenadas registradas
		int xx = MIN(pulsar.x, soltar.x);
		int yy = MIN(pulsar.y, soltar.y);
		int width = std::abs(pulsar.x - soltar.x);
		int height = std::abs(pulsar.y - soltar.y);
		// Hay que comprobar que la plaza seleccionada tenga un tama�o m�nimo para que el programa funcione corecctamente
		if (width <= dimensionesROI || height <= dimensionesROI)
			cout << "Plaza seleccionada demasiado peque�a, vuelva a intentarlo." << endl;
		else
		{
			plaza.push_back(Rect(xx, yy, width, height));
			numeroPlazas++;
		}
		break;
	}

	// Evento soltar bot�n derecho. Utilizado para eliminar las plazas mal definidas
	case CV_EVENT_RBUTTONUP:
		for (unsigned int i = plaza.size(); i > 0; i--)
		{
			// Comprobamos si la coordenada x pertenece a una plaza ya creada
			if (x > plaza[i - 1].x && x < (plaza[i - 1].x + plaza[i - 1].width))
			{
				// Comprobamos si la coordenada y tambi�n se encuentra dentro de una plaza
				if (y > plaza[i - 1].y && y < (plaza[i - 1].y + plaza[i - 1].height))
				{
					// Procedemos a eliminar la plaza seleccionada
					plaza.erase(plaza.begin() + i - 1);
					numeroPlazas--;
					break;
				}
			}
		}
		break;

	default:
		break;
	}
}

/* Funci�n que calcula el umbral para la funci�n threshold din�micamente a partir del nivel de gris de las plazas cuando est�n libres */
int calcular_umbral(Mat plaza)
{
	int max_blanco = 0;
	int nuevo_umbral = 0;

	// Recorremos la imagen en busca del pixel m�s claro distinto de blanco, a partir del cual definimos el nuevo umbral

	// Plazas orientadas verticalmente
	if (plaza.cols <= plaza.rows)
	{
		for (int i = 0; i < plaza.cols; i++)
		{
			uchar *data = plaza.ptr<uchar>(i);

			for (int j = 0; j < plaza.rows; j++)
			{
				if (data[j] < 220 && data[j] > max_blanco)
					max_blanco = data[j];
			}
		}
	}
	// Plazas orientadas horizontalmente
	else
	{
		for (int i = 0; i < plaza.rows; i++)
		{
			uchar *data = plaza.ptr<uchar>(i);

			for (int j = 0; j < plaza.cols; j++)
			{
				if (data[j] < 220 && data[j] > max_blanco)
					max_blanco = data[j];
			}
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