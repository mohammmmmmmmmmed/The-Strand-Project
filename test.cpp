#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <pqxx/pqxx>

using namespace std;

class LogisticRegression
{
public:
    double b0, b1, b2, b3;

    LogisticRegression() : b0(0.0), b1(0.0), b2(0.0), b3(0.0) {}

    double sigmoid(double z)
    {
        return 1.0 / (1.0 + exp(-z));
    }

    void train(const vector<vector<double>> &data, const vector<int> &labels, double learning_rate, int epochs)
    {
        for (int epoch = 0; epoch < epochs; ++epoch)
        {
            for (size_t i = 0; i < data.size(); ++i)
            {
                double z = b0 + b1 * data[i][0] + b2 * data[i][1] + b3 * abs(data[i][4] - data[i][3]);
                double prediction = sigmoid(z);
                double error = labels[i] - prediction;

                b0 += learning_rate * error * prediction * (1 - prediction);
                b1 += learning_rate * error * prediction * (1 - prediction) * data[i][0];
                b2 += learning_rate * error * prediction * (1 - prediction) * data[i][1];
                b3 += learning_rate * error * prediction * (1 - prediction) * abs(data[i][4] - data[i][3]);
            }
        }
    }

    double predict(const vector<double> &features)
    {
        double z = b0 + b1 * features[0] + b2 * features[1] + b3 * abs(features[2] - features[3]);
        return sigmoid(z);
    }
};

void save_data_to_db(const string &conn_str, const vector<vector<double>> &data, const vector<int> &labels)
{
    try
    {
        pqxx::connection C(conn_str);
        if (C.is_open())
        {
            pqxx::work W(C);
            for (size_t i = 0; i < data.size(); ++i)
            {
                string sql = "INSERT INTO training_data (disease_rate, infection_rate, population, ideal_temp, city_temp, label) "
                             "VALUES (" +
                             to_string(data[i][0]) + ", " + to_string(data[i][1]) + ", " + to_string(data[i][2]) + ", " + to_string(data[i][3]) + ", " + to_string(data[i][4]) + ", " + to_string(labels[i]) + ");";
                W.exec(sql);
            }
            W.commit();
            cout << "Training data saved to database.\n";
        }
        else
        {
            cerr << "Can't open database" << endl;
            return;
        }
    }
    catch (const std::exception &e)
    {
        cerr << e.what() << std::endl;
    }
}

void save_prediction_to_db(const string &conn_str, double day, double predicted_disease_rate,
                           double predicted_infection_rate, double prediction_prob)
{
    try
    {
        pqxx::connection C(conn_str);
        if (C.is_open())
        {
            pqxx::work W(C);
            string sql = "INSERT INTO predictions (day, predicted_disease_rate, predicted_infection_rate, prediction_probability) "
                         "VALUES (" +
                         to_string(day) + ", " + to_string(predicted_disease_rate) + ", " + to_string(predicted_infection_rate) + ", " + to_string(prediction_prob) + ");";
            W.exec(sql);
            W.commit();
            cout << "Prediction saved to database for day " << day << ".\n";
        }
        else
        {
            cerr << "Can't open database" << endl;
            return;
        }
    }
    catch (const std::exception &e)
    {
        cerr << e.what() << std::endl;
    }
}

void get_user_data(vector<vector<double>> &data, vector<int> &labels)
{
    int num_points;
    cout << "Enter the number of data points: ";
    cin >> num_points;

    for (int i = 0; i < num_points; ++i)
    {
        vector<double> features(5);
        int label;
        cout << "Enter features (5 values) for data point " << (i + 1) << ": ";
        cin >> features[0] >> features[1] >> features[2] >> features[3] >> features[4];
        cout << "Enter label for data point " << (i + 1) << " (0 or 1): ";
        cin >> label;
        data.push_back(features);
        labels.push_back(label);
    }
}

int main()
{
    vector<vector<double>> default_data = {
        {1.5, 0.8, 5000, 25, 30},
        {2.0, 1.1, 5000, 25, 28},
        {3.0, 1.5, 7000, 22, 23},
        {2.5, 1.2, 10000, 25, 25},
        {1.8, 0.9, 6000, 20, 18},
        {2.1, 1.3, 8000, 22, 20},
        {1.7, 1.0, 7500, 23, 21},
        {2.3, 1.4, 9500, 21, 19}};

    vector<int> default_labels = {0, 1, 1, 1, 0, 1, 0, 1};

    vector<vector<double>> data;
    vector<int> labels;

    string conn_str = "dbname=tdb user=postgres password=mohammed85";

    while (true)
    {
        int choice;
        cout << "1. Use default training dataset\n";
        cout << "2. Enter custom training dataset\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        if (choice == 1)
        {
            data = default_data;
            labels = default_labels;
        }
        else if (choice == 2)
        {
            get_user_data(data, labels);
        }
        else
        {
            break;
        }

        save_data_to_db(conn_str, data, labels);

        vector<vector<double>> features;
        for (const auto &entry : data)
        {
            features.push_back({entry[0], entry[1], abs(entry[4] - entry[3]), entry[3]});
        }

        LogisticRegression model;
        model.train(features, labels, 0.01, 1000);

        double disease_rate, infection_rate, population, ideal_temp, city_temp;
        cout << "Enter the population of the zone: ";
        cin >> population;
        cout << "Enter the current disease rate (as a percentage of population): ";
        cin >> disease_rate;
        cout << "Enter the current infection rate (as a percentage of population): ";
        cin >> infection_rate;
        cout << "Enter ideal thriving temperature for the virus (in °C): ";
        cin >> ideal_temp;
        cout << "Enter current city temperature (in °C): ";
        cin >> city_temp;

        vector<double> input = {
            disease_rate,
            infection_rate,
            city_temp,
            ideal_temp};

        int days;
        cout << "Enter the number of days into the future to predict: ";
        cin >> days;

        cout << fixed << setprecision(4);
        cout << "--- Prediction Timeline ---\n";

        for (int day = 0; day <= days; day += 10)
        {
            double growth_rate = 0.1;
            double predicted_disease_rate = disease_rate * exp(growth_rate * day);
            double predicted_infection_rate = infection_rate * exp(growth_rate * day);

            double predicted_disease_people = predicted_disease_rate / 100 * population;
            double predicted_infection_people = predicted_infection_rate / 100 * population;

            double prediction_prob = model.predict(input);

            cout << "Day " << day << ":\n";
            cout << "  Predicted disease rate: " << predicted_disease_rate << " ("
                 << predicted_disease_people << " people, " << (predicted_disease_rate / population * 100) << "% of population)\n";
            cout << "  Predicted infection rate: " << predicted_infection_rate << " ("
                 << predicted_infection_people << " people, " << (predicted_infection_rate / population * 100) << "% of population)\n";
            cout << "  Probability of being a disease zone: " << prediction_prob * 100 << "%\n";

            save_prediction_to_db(conn_str, day, predicted_disease_rate, predicted_infection_rate, prediction_prob);

            input[0] *= exp(growth_rate * 10);
            input[1] *= exp(growth_rate * 10);
        }

        cout << "--- Final Prediction After " << days << " Days ---\n";
        cout << "Final predicted disease rate: " << input[0] << " ("
             << (input[0] / population * 100) << "% of population)\n";
        cout << "Final predicted infection rate: " << input[1] << " ("
             << (input[1] / population * 100) << "% of population)\n";
        cout << "Final probability of being a disease zone: " << model.predict(input) * 100 << "%\n";
        cout << "Classification: " << (model.predict(input) > 0.5 ? "Disease Zone" : "No Disease Zone") << "\n";
    }
        return 0;
    
}