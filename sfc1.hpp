//============================================================================
// Name        : sfc1.cpp
// Author      : xrisam00
// Version     :
// Copyright   : skolska licence
// Description -cz : projekt do predmetu SFC, demonstrace komprese dat pomoci BBP a SBP
// Description -en : projec for school subject soft computing, demonstration of data compression with SBP and BBP
//============================================================================


#ifndef SFC1_HPP
#define SFC1_HPP

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <time.h>       /* time */
#include <vector>
#include<QDebug>

#include <QProgressBar> //for setting gui-progress bar
#include <QLabel>       //for setting gui-text

/** SUPPORTED ACTIVACTION FUNCTIONS*/
enum activationFType{LINEAR/*not implemented*/,SIGMODIAL,BIPOLAR} m_Ftype;

int BAD_ALLOC_COUNT;
/**
 * Generates random weight vectors of desired size while values \in <-0.5,0.5>
 * */
void _generateW(int desiredSize,std::vector<double>& w)
{
    w.reserve(desiredSize);
    for(int i=0; i < desiredSize; ++i)
        w.push_back(((double)std::rand()/(double) RAND_MAX)-0.5);
   // w.push_back(1.0);
}


class Neuron;
class Layer
{
private: //members
    std::vector<Neuron *> m_lay; //layers vector

public: //methods
    Layer(std::vector<Neuron *>& inSamples);
    Layer(Layer * Lminus1,int desiredSize);
    Neuron * operator[](int index){return m_lay[index];}
    int Size (){return m_lay.size();}
    double GetDeltaSum(int index);
    void QuantizeOutput();
    void DequantizeOutput();
    double GetDeltaSum();
    std::vector<Neuron *> & Neurons(){return m_lay;}
    ~Layer();
};

class Neuron
{
private: //members
    std::vector<Neuron *> m_input;
    std::vector<double> m_w;
    double m_output;
    double m_delta;
    int m_quantizedOutput;

public: //methods
    /**
     * """The sigmoid function."""
     * */
    double _sigmoid(double z)
    {
        //qDebug() <<  "sigmo " << 1.0/(1.0+std::exp(-z));
        return 1.0/(1.0+std::exp(-z));
    }

    /**
     * """Derivative of the sigmoid function."""
     * */
    double _sigmoidPrime()
    {
        //qDebug() <<  "sigmopr " << m_output*(1-m_output);
        return m_output*(1-m_output);
    }

    /**
     * """The bipolaroid function."""
     * */
    double _bipolaroid(double z)
    {
        return (1.0-std::exp(-z))/(1.0+std::exp(-z));
    }

    /**
     * """Derivative of the bipolaroid function."""
     * */
    double _bipolaroidPrime(double LAMBDA)
    {
        double tmp = std::exp(LAMBDA*EvalSum());
        return (2*tmp)/((1+tmp) * (1+tmp));
    }

public: //methods
    /***
     * C-tors
     * */
    Neuron(std::vector<double> & initW, Layer * inL, Neuron * unitN)
    {
        m_w.reserve(initW.size());
        m_input.reserve(initW.size());
        for(unsigned i =0; i < initW.size(); ++i)
        {
            if(i == initW.size()-1)
                m_input.push_back(unitN);
            else
                m_input.push_back(inL->operator [](i));

            m_w.push_back(initW[i]);
        }
        m_output = 0.0;
        m_quantizedOutput = 0;
        m_delta = 0.0;
    }
    Neuron(double implicitvalue):m_output(implicitvalue)
    {
        m_output = 0.0;
        m_quantizedOutput = 0;
        m_delta = 0.0;
    }
    void AddInput(Neuron * n)
    {
        m_input.push_back(n);
    }
    /***
     * D-tors
     * */
    ~Neuron(){}
    /**
     * Setters
     * */
    void SetWeight(double weightD, int index)
    {
        m_w[index] += weightD;
    }

    void SetValue(double val)
    {
        m_output = val;
    }

    void SetDelta(double val)
    {
        m_delta = val;
    }
    /**
     * Getters
     * */
    unsigned InputSize()
    {
        return m_input.size();
    }
    double Value()
    {
        return m_output;
    }
    double Omega()
    {
        return m_delta;
    }
    double Weight(int i)
    {
        return m_w[i];
    }


    double EvalSum()
    {
        double sum = 0.0;
        for(unsigned i=0; i<m_w.size();++i)
        {
            sum += m_w[i] * m_input[i]->Value();
        }
        return sum;
    }
    /**
     * Modifiers
     * */
    void EvalOutput(double LAMBDA)
    {
        double sum = EvalSum();
        if(m_Ftype == BIPOLAR)
            m_output = _bipolaroid(LAMBDA*sum);
        else if(m_Ftype == SIGMODIAL)
            m_output = _sigmoid(LAMBDA*sum);
        else
            m_output = 0.0;
    }

    void Quantize()
    {
        m_quantizedOutput = (int)(m_Ftype == BIPOLAR)?
                    (signed char)(m_output*128)
                :
                    (unsigned char)(m_output*256);
    }

    void Dequantize()
    {
        m_output = (double)(m_Ftype == BIPOLAR)?
                    (double)(m_quantizedOutput/128)
                :
                    (double)(m_quantizedOutput/256);
    }
};
/**
 * Layer impl
 * */

double Layer::GetDeltaSum(int index)
{
	double sum = 0.0;
    for(int i = 0; i < Size(); ++i)
	{
		Neuron * n = m_lay[i];
		sum += n->Omega()* n->Weight(index);
	}
	return sum;
}

Layer::Layer(std::vector<Neuron *>& inSamples)
{
    m_lay.reserve(inSamples.size());
	for(unsigned i=0; i< inSamples.size(); ++i)
	{
		m_lay.push_back(inSamples[i]);
	}
}

Layer::Layer(Layer * Lminus1,int desiredSize)
{
     m_lay.reserve(desiredSize);
    if(Lminus1 == NULL)
    {
        for(int i = 0; i < desiredSize; ++i)
        {
            try{
                Neuron * n = new Neuron(0.0);
                m_lay.push_back(n);
            }
            catch (std::bad_alloc& ba)
             {
                ++BAD_ALLOC_COUNT;
            }

        }
        return;
    }

    m_lay.reserve(desiredSize);
    for(int i = 0; i < desiredSize; ++i)
	{
		std::vector<double> w;
        _generateW(Lminus1->Size()+1,w);
        try{
            Neuron * unitN = new Neuron(1.0);
            Neuron * n = new Neuron(w,Lminus1,unitN);
            m_lay.push_back(n);
        }
        catch (std::bad_alloc& ba)
        {
            ++BAD_ALLOC_COUNT;
        }

	}
}

void Layer::QuantizeOutput()
{
	for(unsigned i = 0; i<m_lay.size(); ++i)
	{
		m_lay[i]->Quantize();
	}
}

void Layer::DequantizeOutput()
{
	for(unsigned i = 0; i<m_lay.size(); ++i)
	{
		m_lay[i]->Dequantize();
	}
}

class Bpn
{
private: //members
	double m_learningrate;
	std::vector<Layer * > m_net;
	bool m_learned;

public: //members
	double GlobalError;
    double LAMBDA;
    int STEP;

private: //methods
	Layer * _buildLayer(Layer * prevL, int size)
	{
        try{
            return new Layer(prevL,size);
        }
        catch (std::bad_alloc& ba)
        {
            ++BAD_ALLOC_COUNT;
            return prevL;
        }
	}

	void _quantize()
	{
		m_net[m_net.size()-2]->QuantizeOutput();
	}

	void _transmit()
	{;;;}

	void _dequantize()
	{
		m_net[m_net.size()-2]->DequantizeOutput();
	}

    Neuron * _UpdateNeuron(int layer, int index, double LAMBDA)
	{
		Neuron * n = m_net[layer]->operator [](index);
        n->EvalOutput(LAMBDA);
		return n;
	}
	void _TakeoverInputLayer(Layer * L)
	{
        Neuron * n;
        Layer * inL = m_net[0];
        for(int i =0; i < inL->Size(); ++i)
		{
            n = inL->operator [](i);
            n->SetValue(L->operator [](i)->Value());
		}
	}

public: //methods
	/***
	 * C-tor
	 * */
    Bpn(double learningRate,int inputSize, std::vector<int> & middleCounts ,activationFType fType,double lambda)
	{
        BAD_ALLOC_COUNT = 0;
        LAMBDA = lambda;
        srand(time(NULL));
		m_Ftype = fType;
		m_learningrate = learningRate;
		m_learned = false;

		Layer * prevLayer = NULL;
		//create input layer
		m_net.push_back(prevLayer = new Layer(NULL,inputSize));
		//create midlle layers
        for(unsigned i=0; i< middleCounts.size(); ++i)
		{
			Layer * nextL = _buildLayer(prevLayer,middleCounts[i]);
			m_net.push_back(nextL);
			prevLayer = nextL;
		}
		//create output layer
		Layer * actLayer = _buildLayer(prevLayer,inputSize);
        m_net.push_back(actLayer);
	}
	/***
	 * D-tor
	 * */



    void Learn(std::vector<Layer *> &  inSamples, double DesiredError, int MAXSTEP,QLabel * lbtext, QProgressBar * pbstep,QProgressBar * pbsample)
	{

		/*
		  do
		     forEach training example ex
		        prediction = neural-net-output(network, ex)  // forward pass
		        actual = teacher-output(ex)
		        compute error (prediction - actual) at the output units
		        compute \Delta w_h for all weights from hidden layer to output layer  // backward pass
		        compute \Delta w_i for all weights from input layer to hidden layer   // backward pass continued
		        update network weights // input layer not modified by error estimate
		  until all examples classified correctly or another stopping criterion satisfied
		  */
        lbtext->setText("LEARNING ON BLOCKS COUNT SAMPLES, PLEASE WAIT...");
        lbtext->show();

        pbstep->setRange(0,MAXSTEP+10);
        pbsample->setRange(0,inSamples.size()+10);
        STEP = 0;
		do
		{

        pbstep->setValue(STEP);
        pbstep->show();
		GlobalError = 0.0;
        ++STEP;


		for(unsigned i = 0; i < inSamples.size(); ++i)
		{
            /*if(i % 128 == 0)
            {
                pbsample->setValue(i);
                pbsample->show();
            }*/


			//set layer as input layer
			_TakeoverInputLayer(inSamples[i]);

            if(BAD_ALLOC_COUNT)
            {
                GlobalError =-1;
                STEP = -1;
                return;
            }
			//for all layers
            for(unsigned l=1; l < m_net.size();++l)
			{
				Layer * L = m_net[l];
				//for all neurons
                for(int j=0; j < L->Size();++j)
				{
					Neuron * n = L->operator [](j);
                    n->EvalOutput(LAMBDA);
				}
			}

			double localError=0.0;
			//for all neurons on output layer
			Layer * LastL = m_net[m_net.size()-1];
			Layer * FirstL = m_net[0];
            for(int j=0; j < LastL->Size();++j)
			{
				Neuron * nlastL = LastL->operator [](j);
				Neuron * nfirstL = FirstL->operator [](j);

                double val = nfirstL->Value() - nlastL->Value();
                localError += val * val;

                if(m_Ftype == SIGMODIAL)
                {
                    nlastL->SetDelta(val * LAMBDA * nlastL->_sigmoidPrime());
                }
                else if(m_Ftype == BIPOLAR)
                {
                    nlastL->SetDelta(val * LAMBDA * nlastL->_bipolaroidPrime(LAMBDA));
                }
			}

			GlobalError += 0.5 * localError;
			//backpropagation deltas
			//for all layers except output in reversed order
            for(unsigned l=m_net.size()-2; l>0;--l)
			{
				Layer * L = m_net[l];
				Layer * Labove = m_net[l+1];
				//for all neurons on da layer
                for(int j=0; j < L->Size();j++ )
				{
					Neuron * n = L->operator [](j);
					double val = Labove->GetDeltaSum(j);
                    if(m_Ftype == SIGMODIAL)
                    {
                        n->SetDelta(val * LAMBDA * n->_sigmoidPrime());
                    }
                    else if(m_Ftype == BIPOLAR)
                    {
                        n->SetDelta(val * LAMBDA * n->_bipolaroidPrime(LAMBDA));
                    }
				}
			}
			//eval new weights
			//for all layers(starting with 2nd cos 1st is input)
			for(unsigned l=1; l < m_net.size();++l)
			{
				Layer * L = m_net[l];
				Layer * PrevL = m_net[l-1];
				//for all its neurons
                for(int j=0; j < L->Size();j++ )
				{
					Neuron * n = L->operator [](j);
					//for all neuron weights
                    for(unsigned i=0; i < n->InputSize();i++ )
					{
                        double deltaW;
                        if(i== n->InputSize()-1)
                           deltaW = m_learningrate * n->Omega() ;
                        else
                            deltaW = m_learningrate * n->Omega() * PrevL->operator [](i)->Value();

                        n->SetWeight(deltaW,i);
					}

				}
			}
		} //end for all traingin set
        }while(STEP < MAXSTEP && GlobalError >= DesiredError); //end while
		m_learned = true;
	}
	void Step(std::vector<Neuron *> & newInputs,std::vector<Neuron *> & newOutputs)
	{
		if(!m_learned) return;

        if(BAD_ALLOC_COUNT)
        {
            GlobalError =-1;
            STEP = -1;
            return;
        }

        for(unsigned i=0; i< m_net.size()-1;++i)//for all layers axcept output
		{
			for(int n=0; n < m_net[i]->Size();++n)//for all neurons on the layer
			{
				if(i == 0) // if input layer
				{
					//set new inputs in the input layer
					m_net[i]->operator [](n)->SetValue(newInputs[n]->Value());
					continue;
				}

				//eval new input based on previous layer
                _UpdateNeuron(i,n,LAMBDA);
			}
		}
		//dequantize layer before last one
        //_dequantize();
		//trasmit
        //_transmit();
		//qantize the same layer
		//_quantize();
		//step last layer
		for(int n=0; n < m_net[m_net.size()-1]->Size();++n)//for all neurons on the last layer
		{
			//eval new input based on previous layer
            newOutputs.push_back(_UpdateNeuron(m_net.size()-1,n,LAMBDA));
		}

	}

};
#else
;;;
#endif
